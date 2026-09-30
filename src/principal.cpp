#include <gst/gst.h>
#include <gst/video/video.h>
#include <chrono>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;
static volatile std::sig_atomic_t interrompido = 0;

// O sinal apenas altera uma flag; chamadas GStreamer ficam na thread principal.
void SolicitarParada(int)
{
    interrompido = 1;
}

struct Medicao
{
    std::string nome;
    fs::path imagem;
    guint64 quadros = 0;
    GstClockTime primeiro = GST_CLOCK_TIME_NONE;
    GstClockTime ultimo = GST_CLOCK_TIME_NONE;
    GstVideoInfo video{};
    bool temFormato = false;
    bool imagemGravada = false;
    bool falhou = false;
};

// Salva RGB em PPM e cinza em PGM, respeitando o stride de cada linha.
bool SalvarImagem(GstBuffer* buffer, Medicao& medicao)
{
    GstVideoFrame quadro;
    if (!gst_video_frame_map(&quadro, &medicao.video, buffer, GST_MAP_READ))
    {
        return false;
    }
    const bool rgb = GST_VIDEO_INFO_FORMAT(&medicao.video) == GST_VIDEO_FORMAT_RGB;
    const int largura = GST_VIDEO_INFO_WIDTH(&medicao.video);
    const int altura = GST_VIDEO_INFO_HEIGHT(&medicao.video);
    std::ofstream arquivo(medicao.imagem, std::ios::binary);
    arquivo << (rgb ? "P6\n" : "P5\n") << largura << ' ' << altura << "\n255\n";
    const auto* dados = static_cast<const char*>(GST_VIDEO_FRAME_PLANE_DATA(&quadro, 0));
    const int passo = GST_VIDEO_FRAME_PLANE_STRIDE(&quadro, 0);
    for (int linha = 0; linha < altura && arquivo; ++linha)
    {
        arquivo.write(dados + linha * passo, largura * (rgb ? 3 : 1));
    }
    arquivo.close();
    const bool sucesso = !arquivo.fail();
    gst_video_frame_unmap(&quadro);
    return sucesso;
}

// Mede buffers reais antes de qualquer conversao exigida pelo dispositivo de exibicao.
GstPadProbeReturn Observar(GstPad* pad, GstPadProbeInfo* informacao, gpointer dados)
{
    auto& medicao = *static_cast<Medicao*>(dados);
    GstBuffer* buffer = GST_PAD_PROBE_INFO_BUFFER(informacao);
    if (!buffer)
    {
        return GST_PAD_PROBE_OK;
    }
    if (!medicao.temFormato)
    {
        GstCaps* caps = gst_pad_get_current_caps(pad);
        medicao.temFormato = caps && gst_video_info_from_caps(&medicao.video, caps);
        if (caps)
        {
            gst_caps_unref(caps);
        }
        if (!medicao.temFormato)
        {
            medicao.falhou = true;
            return GST_PAD_PROBE_OK;
        }
    }
    ++medicao.quadros;
    if (medicao.quadros == 1)
    {
        medicao.primeiro = GST_BUFFER_PTS(buffer);
    }
    medicao.ultimo = GST_BUFFER_PTS(buffer);
    if (!medicao.imagem.empty() && !medicao.imagemGravada && !medicao.falhou)
    {
        medicao.imagemGravada = SalvarImagem(buffer, medicao);
        medicao.falhou = !medicao.imagemGravada;
    }
    return GST_PAD_PROBE_OK;
}

// Liga a medicao a um capsfilter nomeado e libera as referencias temporarias.
void InstalarMedicao(GstElement* pipeline, const char* nome, Medicao& medicao)
{
    GstElement* elemento = gst_bin_get_by_name(GST_BIN(pipeline), nome);
    if (!elemento)
    {
        throw std::runtime_error("Ponto de medicao ausente.");
    }
    GstPad* pad = gst_element_get_static_pad(elemento, "src");
    gst_pad_add_probe(pad, GST_PAD_PROBE_TYPE_BUFFER, Observar, &medicao, nullptr);
    gst_object_unref(pad);
    gst_object_unref(elemento);
}

// Serializa dados observados; FPS das caps e cadencia medida sao conceitos distintos.
void EscreverMedicao(std::ostream& destino, const Medicao& medicao)
{
    const double fpsMedido = medicao.quadros > 1 && GST_CLOCK_TIME_IS_VALID(medicao.primeiro) &&
                                     GST_CLOCK_TIME_IS_VALID(medicao.ultimo) &&
                                     medicao.ultimo > medicao.primeiro
                                 ? static_cast<double>(medicao.quadros - 1) * GST_SECOND /
                                       (medicao.ultimo - medicao.primeiro)
                                 : 0;
    destino << "{\"largura\":" << GST_VIDEO_INFO_WIDTH(&medicao.video)
            << ",\"altura\":" << GST_VIDEO_INFO_HEIGHT(&medicao.video) << ",\"formato\":\""
            << gst_video_format_to_string(GST_VIDEO_INFO_FORMAT(&medicao.video))
            << "\",\"fps_numerador\":" << GST_VIDEO_INFO_FPS_N(&medicao.video)
            << ",\"fps_denominador\":" << GST_VIDEO_INFO_FPS_D(&medicao.video)
            << ",\"quadros\":" << medicao.quadros << ",\"fps_por_pts\":" << fpsMedido << '}';
}

// Inicializa, monta, executa, acompanha o bus e encerra a pipeline com limpeza garantida.
int main(int argc, char** argv)
{
    GstElement* pipeline = nullptr;
    GstBus* bus = nullptr;
    int resultado = 1;
    // Permanecem vivos ate a parada das threads da pipeline, inclusive em excecoes.
    Medicao original;
    original.nome = "original";
    Medicao processado;
    processado.nome = "processado";
    try
    {
        gst_init(&argc, &argv);
        bool semJanela = false;
        int segundos = 15;
        fs::path pasta;
        for (int indice = 1; indice < argc; ++indice)
        {
            const std::string argumento = argv[indice];
            if (argumento == "--ajuda")
            {
                std::cout
                    << "Uso: pipeline_multimidia [--sem-janela] [--segundos 1..300] [--exportar PASTA]\n"
                    << "Padrao: duas janelas por 15 segundos. Ctrl+C solicita EOS.\n"
                    << "Exportar: original.ppm, processado.pgm e relatorio.json. Use pasta nova.\n";
                return 0;
            }
            else if (argumento == "--sem-janela")
            {
                semJanela = true;
            }
            else if (argumento == "--segundos" && indice + 1 < argc)
            {
                std::string valor = argv[++indice];
                std::size_t lidos = 0;
                segundos = std::stoi(valor, &lidos);
                if (lidos != valor.size() || segundos < 1 || segundos > 300)
                {
                    throw std::runtime_error("Segundos deve ser inteiro entre 1 e 300.");
                }
            }
            else if (argumento == "--exportar" && indice + 1 < argc)
            {
                pasta = argv[++indice];
            }
            else
            {
                throw std::runtime_error("Argumento invalido ou incompleto: " + argumento);
            }
        }
        if (!pasta.empty())
        {
            fs::create_directories(pasta);
            for (const auto* nome : {"original.ppm", "processado.pgm", "relatorio.json"})
            {
                if (fs::exists(pasta / nome))
                {
                    throw std::runtime_error("Saida ja existe. Escolha uma pasta nova.");
                }
            }
            original.imagem = pasta / "original.ppm";
            processado.imagem = pasta / "processado.pgm";
        }

        const std::string saida =
            semJanela ? "fakesink sync=false" : "videoconvert ! autovideosink sync=true";
        // Uma unica fonte garante o mesmo conteudo e a mesma origem temporal nos ramos.
        const std::string descricao =
            "videotestsrc pattern=smpte horizontal-speed=4 num-buffers=" +
            std::to_string(segundos * 30) +
            " ! video/x-raw,format=RGB,width=640,height=360,framerate=30/1,pixel-aspect-ratio=1/1"
            " ! tee name=divisor "
            "divisor. ! queue ! capsfilter name=antes caps=video/x-raw,format=RGB,width=640,height=360,framerate=30/1 ! " +
            saida +
            " divisor. ! queue ! videoscale ! videorate ! videoconvert"
            " ! capsfilter name=depois caps=video/x-raw,format=GRAY8,width=320,height=180,framerate=10/1,pixel-aspect-ratio=1/1 ! " +
            saida;
        GError* erro = nullptr;
        pipeline = gst_parse_launch(descricao.c_str(), &erro);
        if (erro)
        {
            const std::string mensagem = erro->message;
            g_error_free(erro);
            throw std::runtime_error("Falha na montagem (confira plugins): " + mensagem);
        }
        if (!pipeline)
        {
            throw std::runtime_error("Pipeline nao criada.");
        }
        InstalarMedicao(pipeline, "antes", original);
        InstalarMedicao(pipeline, "depois", processado);
        bus = gst_element_get_bus(pipeline);
        std::signal(SIGINT, SolicitarParada);
        gchar* versao = gst_version_string();
        std::cout << versao << '\n';
        g_free(versao);
        std::cout
            << "ANTES: 640x360 | 30 FPS | RGB (colorido)\n"
            << "DEPOIS: 320x180 | 10 FPS | GRAY8 (cinza)\n"
            << (semJanela
                    ? "Modo sem janela: processamento sem espera de relogio.\n"
                    : "Duas janelas: organize lado a lado. Aguarde EOS ou pressione Ctrl+C.\n");
        if (gst_element_set_state(pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE)
        {
            throw std::runtime_error("Nao foi possivel iniciar a pipeline.");
        }
        const auto limite = std::chrono::steady_clock::now() + std::chrono::seconds(segundos + 20);
        bool eosSolicitado = false;
        bool eosRecebido = false;
        while (!eosRecebido)
        {
            if (interrompido && !eosSolicitado)
            {
                eosSolicitado = true;
                if (!gst_element_send_event(pipeline, gst_event_new_eos()))
                {
                    throw std::runtime_error("Nao foi possivel solicitar EOS.");
                }
            }
            if (std::chrono::steady_clock::now() > limite)
            {
                throw std::runtime_error("Tempo limite excedido sem EOS.");
            }
            GstMessage* mensagem = gst_bus_timed_pop_filtered(
                bus, 100 * GST_MSECOND,
                static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
            if (!mensagem)
            {
                continue;
            }
            if (GST_MESSAGE_TYPE(mensagem) == GST_MESSAGE_ERROR)
            {
                GError* falha = nullptr;
                gchar* depuracao = nullptr;
                gst_message_parse_error(mensagem, &falha, &depuracao);
                const std::string texto =
                    std::string(GST_OBJECT_NAME(mensagem->src)) + ": " + falha->message;
                if (depuracao)
                {
                    std::cerr << depuracao << '\n';
                }
                g_clear_error(&falha);
                g_free(depuracao);
                gst_message_unref(mensagem);
                throw std::runtime_error(texto);
            }
            eosRecebido = true;
            gst_message_unref(mensagem);
        }
        // Parar antes de ler os contadores evita concorrencia com os probes.
        gst_element_set_state(pipeline, GST_STATE_NULL);
        if (!original.temFormato || !processado.temFormato || original.falhou || processado.falhou)
        {
            throw std::runtime_error("Falha na medicao ou gravacao de imagem.");
        }
        for (const auto* medicao : {&original, &processado})
        {
            std::cout << medicao->nome << ": ";
            EscreverMedicao(std::cout, *medicao);
            std::cout << '\n';
        }
        if (!pasta.empty())
        {
            std::ofstream relatorio(pasta / "relatorio.json");
            relatorio << "{\"original\":";
            EscreverMedicao(relatorio, original);
            relatorio << ",\"processado\":";
            EscreverMedicao(relatorio, processado);
            relatorio << "}\n";
            relatorio.close();
            if (relatorio.fail())
            {
                throw std::runtime_error("Falha ao gravar relatorio.");
            }
        }
        std::cout << "EOS recebido. Recursos liberados.\n";
        resultado = 0;
    }
    catch (const std::exception& erro)
    {
        std::cerr << "Erro: " << erro.what() << '\n';
    }
    if (pipeline)
    {
        gst_element_set_state(pipeline, GST_STATE_NULL);
    }
    if (bus)
    {
        gst_object_unref(bus);
    }
    if (pipeline)
    {
        gst_object_unref(pipeline);
    }
    return resultado;
}
