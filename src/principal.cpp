#include <gst/gst.h>
#include <gst/video/video.h>

#include <chrono>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#include <glib/gwin32.h>
#endif

namespace fs = std::filesystem;

// O manipulador de Ctrl+C e o laco principal compartilham somente esta flag.
// sig_atomic_t permite altera-la em um sinal sem executar operacoes complexas ali.
static volatile std::sig_atomic_t interrompido = 0;

/**
 * Registra um pedido de encerramento feito por Ctrl+C.
 *
 * Nao chama o GStreamer dentro do manipulador de sinal. O laco principal le
 * esta flag e envia EOS (fim do fluxo), permitindo encerrar a pipeline.
 */
void SolicitarParada(int)
{
    interrompido = 1;
}

/**
 * Guarda os dados observados em um ramo: original ou processado.
 *
 * Cada probe atualiza sua propria instancia na thread de streaming. A thread
 * principal so le o resultado depois de colocar a pipeline no estado NULL.
 */
struct Medicao
{
    // Identificacao do ramo e destino opcional do primeiro quadro exportado.
    std::string nome;
    fs::path imagem;

    // Contagem e timestamps de apresentacao; GST_CLOCK_TIME_NONE indica ausencia.
    guint64 quadros = 0;
    GstClockTime primeiro = GST_CLOCK_TIME_NONE;
    GstClockTime ultimo = GST_CLOCK_TIME_NONE;

    // Formato realmente negociado: dimensoes, representacao dos pixels e FPS.
    GstVideoInfo video{};
    bool temFormato = false;

    // Estado da exportacao. Uma falha fica registrada para a thread principal.
    bool imagemGravada = false;
    bool falhou = false;
};

/**
 * Exporta um quadro bruto sem depender de codecs de imagem.
 *
 * RGB usa PPM/P6 (tres bytes por pixel); GRAY8 usa PGM/P5 (um byte por pixel).
 * O mapeamento fornece acesso somente de leitura ao buffer do GStreamer.
 * Retorna false se o buffer nao puder ser mapeado ou o arquivo nao for gravado.
 */
bool SalvarImagem(GstBuffer* buffer, Medicao& medicao)
{
    // Mapear nao transfere a propriedade do buffer: apenas permite ler seus pixels.
    GstVideoFrame quadro;

    if (!gst_video_frame_map(&quadro, &medicao.video, buffer, GST_MAP_READ))
    {
        return false;
    }

    // As caps dos pontos de medicao limitam os formatos a RGB ou GRAY8.
    const bool rgb = GST_VIDEO_INFO_FORMAT(&medicao.video) == GST_VIDEO_FORMAT_RGB;
    const int largura = GST_VIDEO_INFO_WIDTH(&medicao.video);
    const int altura = GST_VIDEO_INFO_HEIGHT(&medicao.video);

    // O cabecalho descreve formato, dimensoes e valor maximo de cada amostra.
    std::ofstream arquivo(medicao.imagem, std::ios::binary);
    arquivo << (rgb ? "P6\n" : "P5\n") << largura << ' ' << altura << "\n255\n";

    // Stride e a distancia entre linhas em memoria. Pode incluir alinhamento;
    // por isso gravamos apenas os pixels ativos e pulamos o eventual padding.
    const auto* dados = static_cast<const char*>(GST_VIDEO_FRAME_PLANE_DATA(&quadro, 0));
    const int passo = GST_VIDEO_FRAME_PLANE_STRIDE(&quadro, 0);

    for (int linha = 0; linha < altura && arquivo; ++linha)
    {
        arquivo.write(dados + linha * passo, largura * (rgb ? 3 : 1));
    }

    // Fechar antes de testar fail() tambem detecta falhas ao descarregar a escrita.
    arquivo.close();
    const bool sucesso = !arquivo.fail();
    gst_video_frame_unmap(&quadro);

    return sucesso;
}

/**
 * Observa cada buffer que passa pelo pad, sem modificar nem remover a midia.
 *
 * O probe fica na saida do capsfilter: mede o resultado do processamento antes
 * de outra conversao eventualmente exigida pelo dispositivo de exibicao.
 * GST_PAD_PROBE_OK deixa o fluxo seguir normalmente para o proximo elemento.
 */
GstPadProbeReturn Observar(GstPad* pad, GstPadProbeInfo* informacao, gpointer dados)
{
    auto& medicao = *static_cast<Medicao*>(dados);
    GstBuffer* buffer = GST_PAD_PROBE_INFO_BUFFER(informacao);

    if (!buffer)
    {
        return GST_PAD_PROBE_OK;
    }

    // Neste experimento as caps sao fixas. Basta consultar o formato no primeiro
    // buffer; uma pipeline que renegociasse formatos exigiria atualizar essa leitura.
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

    // PTS mede tempo de midia, nao o tempo gasto pela CPU para processar o quadro.
    // O primeiro e o ultimo PTS permitem calcular a cadencia observada no ramo.
    ++medicao.quadros;

    if (medicao.quadros == 1)
    {
        medicao.primeiro = GST_BUFFER_PTS(buffer);
    }

    medicao.ultimo = GST_BUFFER_PTS(buffer);

    // Exportamos apenas o primeiro quadro. Isso gera uma evidencia estatica;
    // nao representa gravacao de um video completo.
    if (!medicao.imagem.empty() && !medicao.imagemGravada && !medicao.falhou)
    {
        medicao.imagemGravada = SalvarImagem(buffer, medicao);
        medicao.falhou = !medicao.imagemGravada;
    }

    return GST_PAD_PROBE_OK;
}

/**
 * Instala um observador no pad de saida do capsfilter "antes" ou "depois".
 *
 * A Medicao passada por referencia precisa permanecer viva durante a execucao.
 * As referencias temporarias ao elemento e ao pad sao liberadas ao final;
 * a pipeline continua sendo responsavel pelos elementos que contem.
 */
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

/**
 * Liga o primeiro fluxo de video descoberto no arquivo ao processamento.
 *
 * Um arquivo pode conter audio, video e legendas. uridecodebin escolhe os
 * demultiplexadores e decodificadores instalados; seus pads aparecem somente
 * quando os fluxos sao identificados. Por isso a ligacao ocorre em pad-added.
 * Audio e legendas sao ignorados: esta atividade processa apenas o video.
 */
void ConectarVideo(GstElement* leitor, GstPad* novoPad, gpointer dados)
{
    auto* entrada = static_cast<GstElement*>(dados);
    GstCaps* caps = gst_pad_get_current_caps(novoPad);

    if (!caps || gst_caps_is_empty(caps))
    {
        if (caps)
        {
            gst_caps_unref(caps);
        }

        return;
    }

    const GstStructure* formato = gst_caps_get_structure(caps, 0);
    const bool ehVideo = g_str_has_prefix(gst_structure_get_name(formato), "video/x-raw");
    gst_caps_unref(caps);

    if (!ehVideo)
    {
        return;
    }

    GstPad* destino = gst_element_get_static_pad(entrada, "sink");

    // Se houver mais de uma faixa de video, preservar a primeira conectada.
    if (!gst_pad_is_linked(destino))
    {
        const GstPadLinkReturn resultado = gst_pad_link(novoPad, destino);

        if (resultado != GST_PAD_LINK_OK)
        {
            GST_ELEMENT_ERROR(leitor, CORE, NEGOTIATION,
                              ("Nao foi possivel conectar o video decodificado."),
                              ("Codigo de ligacao: %d", resultado));
        }
    }

    gst_object_unref(destino);
}

/**
 * Escreve o resultado de um ramo em JSON, no terminal ou em um arquivo.
 *
 * O FPS das caps e o valor negociado. O FPS por PTS usa dados observados:
 * N quadros delimitam N - 1 intervalos. GST_SECOND converte nanossegundos
 * para segundos. Sem intervalo temporal valido, o resultado medido fica zero.
 */
void EscreverMedicao(std::ostream& destino, const Medicao& medicao)
{
    const bool temIntervaloValido =
        medicao.quadros > 1 && GST_CLOCK_TIME_IS_VALID(medicao.primeiro) &&
        GST_CLOCK_TIME_IS_VALID(medicao.ultimo) && medicao.ultimo > medicao.primeiro;

    double fpsMedido = 0;

    if (temIntervaloValido)
    {
        const double quantidadeIntervalos = static_cast<double>(medicao.quadros - 1);
        const GstClockTime duracao = medicao.ultimo - medicao.primeiro;

        fpsMedido = quantidadeIntervalos * GST_SECOND / duracao;
    }

    destino << "{\"largura\":" << GST_VIDEO_INFO_WIDTH(&medicao.video)
            << ",\"altura\":" << GST_VIDEO_INFO_HEIGHT(&medicao.video) << ",\"formato\":\""
            << gst_video_format_to_string(GST_VIDEO_INFO_FORMAT(&medicao.video))
            << "\",\"fps_numerador\":" << GST_VIDEO_INFO_FPS_N(&medicao.video)
            << ",\"fps_denominador\":" << GST_VIDEO_INFO_FPS_D(&medicao.video)
            << ",\"quadros\":" << medicao.quadros << ",\"fps_por_pts\":" << fpsMedido << '}';
}

/**
 * Coordena a aplicacao, da leitura dos argumentos ate a liberacao dos recursos.
 *
 * Fluxo de estudo: configuracao -> montagem -> observacao -> reproducao ->
 * mensagens do bus -> relatorio -> limpeza. Retorna 0 em sucesso e 1 em falha.
 * Os elementos multimidia fazem o processamento; main controla sua execucao.
 */
int main(int argc, char** argv)
{
#ifdef _WIN32
    // O Windows guarda a linha de comando em Unicode. A GLib fornece uma copia
    // UTF-8 para aceitar caminhos com espacos e acentos sem depender do terminal.
    std::unique_ptr<gchar*, decltype(&g_strfreev)> argumentosWindows(g_win32_get_command_line(),
                                                                     g_strfreev);
    argv = argumentosWindows.get();
    argc = static_cast<int>(g_strv_length(argv));
#endif

    GstElement* pipeline = nullptr;
    GstBus* bus = nullptr;
    int resultado = 1;

    // As medicoes ficam fora do try para sobreviverem ate a parada das threads,
    // inclusive quando uma excecao interrompe a execucao.
    Medicao original;
    original.nome = "original";

    Medicao processado;
    processado.nome = "processado";

    try
    {
        // 1. Inicializar o framework e interpretar as opcoes do programa.
        // As opcoes pertencem a esta aplicacao. A configuracao de depuracao do
        // GStreamer continua disponivel pela variavel de ambiente GST_DEBUG.
        gst_init(nullptr, nullptr);

        bool semJanela = false;
        int segundos = 15;
        int inicio = 0;
        fs::path pasta;
        fs::path video;

        for (int indice = 1; indice < argc; ++indice)
        {
            const std::string argumento = argv[indice];

            if (argumento == "--ajuda")
            {
                std::cout
                    << "Uso: pipeline_multimidia --video ARQUIVO [--sem-janela] "
                       "[--segundos 1..300] [--inicio 0..3600] [--exportar PASTA]\n"
                    << "Abre um video existente. Reproduz ate 15 segundos por padrao.\n"
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

                // lidos impede aceitar entradas parciais como "10abc".
                segundos = std::stoi(valor, &lidos);

                if (lidos != valor.size() || segundos < 1 || segundos > 300)
                {
                    throw std::runtime_error("Segundos deve ser inteiro entre 1 e 300.");
                }
            }
            else if (argumento == "--exportar" && indice + 1 < argc)
            {
                pasta = fs::u8path(argv[++indice]);
            }
            else if (argumento == "--inicio" && indice + 1 < argc)
            {
                const std::string valor = argv[++indice];
                std::size_t lidos = 0;
                inicio = std::stoi(valor, &lidos);

                if (lidos != valor.size() || inicio < 0 || inicio > 3600)
                {
                    throw std::runtime_error("Inicio deve ser inteiro entre 0 e 3600.");
                }
            }
            else if (argumento == "--video" && indice + 1 < argc)
            {
                video = fs::u8path(argv[++indice]);
            }
            else
            {
                throw std::runtime_error("Argumento invalido ou incompleto: " + argumento);
            }
        }

        if (video.empty() || !fs::is_regular_file(video))
        {
            throw std::runtime_error("Informe --video com o caminho de um arquivo existente.");
        }

        // 2. Preparar a exportacao opcional, preservando arquivos anteriores.
        // O caminho e tratado pelo C++; nao e inserido no texto da pipeline.
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

        // 3. Escolher como consumir os quadros de cada ramo.
        // Na tela, sync=true respeita o relogio de reproducao. Sem janela,
        // fakesink consome buffers sem espera, mantendo os timestamps da midia.
        const std::string saida =
            semJanela ? "fakesink sync=false" : "videoconvert ! autovideosink sync=true";

        // 4. Descrever a pipeline. Cada "!" conecta elementos consecutivos.
        // O tee distribui a mesma fonte aos dois ramos. Cada queue fornece
        // uma fila e uma thread de streaming para seu respectivo ramo.
        const std::string descricao =

            // O leitor fica inicialmente separado da entrada. ConectarVideo
            // faz a ligacao quando o decoder revelar o primeiro pad de video.
            "uridecodebin name=leitor "
            "queue name=entrada ! videoconvert ! video/x-raw,format=RGB"
            " ! tee name=divisor "

            // Ramo original: conserva resolucao, FPS e representacao RGB.
            "divisor. ! queue ! capsfilter name=antes "
            "caps=video/x-raw,format=RGB ! " +
            saida +

            // Ramo processado: escala espacial, cadencia temporal e cor.
            // O capsfilter exige o resultado; os conversores realizam a mudanca.
            " divisor. ! queue ! videoscale ! videorate ! videoconvert"
            " ! capsfilter name=depois "
            "caps=video/x-raw,format=GRAY8,width=320,height=180,"
            "framerate=10/1,pixel-aspect-ratio=1/1 ! " +
            saida;

        // O parser cria e conecta os elementos pela API. Nenhum comando externo
        // gst-launch e executado. GError tambem pode sinalizar montagem parcial.
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

        // Converter o caminho local em URI escapa espacos e caracteres especiais.
        // A propriedade e definida pela API, sem inserir o caminho na pipeline.
        gchar* uri = gst_filename_to_uri(fs::absolute(video).u8string().c_str(), &erro);

        if (!uri)
        {
            const std::string mensagem = erro ? erro->message : "Caminho invalido.";
            g_clear_error(&erro);
            throw std::runtime_error(mensagem);
        }

        GstElement* leitor = gst_bin_get_by_name(GST_BIN(pipeline), "leitor");
        GstElement* entrada = gst_bin_get_by_name(GST_BIN(pipeline), "entrada");

        g_object_set(leitor, "uri", uri, nullptr);
        g_signal_connect(leitor, "pad-added", G_CALLBACK(ConectarVideo), entrada);

        // A pipeline mantem os elementos vivos durante as chamadas do callback.
        g_free(uri);
        gst_object_unref(entrada);
        gst_object_unref(leitor);

        // 5. Preparar o canal de mensagens e o pedido de parada pelo terminal.
        // O bus carrega mensagens de controle; os pixels viajam pelos pads.
        bus = gst_element_get_bus(pipeline);
        std::signal(SIGINT, SolicitarParada);

        gchar* versao = gst_version_string();
        std::cout << versao << '\n';
        g_free(versao);

        std::cout
            << "ANTES: resolucao e FPS do arquivo | RGB para comparar os pixels\n"
            << "DEPOIS: 320x180 | 10 FPS | GRAY8 (cinza)\n"
            << (semJanela
                    ? "Modo sem janela: processamento sem espera de relogio.\n"
                    : "Duas janelas: organize lado a lado. Aguarde EOS ou pressione Ctrl+C.\n");

        // 6. Preparar o primeiro quadro e limitar o trecho pelo tempo da midia.
        // O seek com inicio e fim funciona tambem no modo sem janela: nao depende
        // da velocidade da CPU. Arquivos curtos terminam naturalmente antes.
        gst_element_set_state(pipeline, GST_STATE_PAUSED);

        if (gst_element_get_state(pipeline, nullptr, nullptr, 15 * GST_SECOND) !=
            GST_STATE_CHANGE_SUCCESS)
        {
            throw std::runtime_error(
                "Nao foi possivel abrir o video. Confira o arquivo e os codecs instalados.");
        }

        // Instalar os probes depois do primeiro preroll evita contar o quadro
        // de preparacao. O seek volta ao inicio e os probes medem esse novo fluxo.
        InstalarMedicao(pipeline, "antes", original);
        InstalarMedicao(pipeline, "depois", processado);

        if (!gst_element_seek(
                pipeline, 1.0, GST_FORMAT_TIME,
                static_cast<GstSeekFlags>(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE),
                GST_SEEK_TYPE_SET, static_cast<gint64>(inicio) * GST_SECOND, GST_SEEK_TYPE_SET,
                static_cast<gint64>(inicio + segundos) * GST_SECOND))
        {
            throw std::runtime_error("O arquivo nao permite delimitar o trecho de reproducao.");
        }

        // Iniciar a reproducao. A transicao pode terminar de forma assincrona;
        // falhas posteriores sao comunicadas pelo bus dentro do laco abaixo.
        if (gst_element_set_state(pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE)
        {
            throw std::runtime_error("Nao foi possivel iniciar a pipeline.");
        }

        // 7. Esperar EOS ou erro sem bloquear indefinidamente.
        // A margem de 20 segundos permite preparar o dispositivo e drenar filas.
        const auto limite = std::chrono::steady_clock::now() + std::chrono::seconds(segundos + 20);
        bool eosSolicitado = false;
        bool eosRecebido = false;

        while (!eosRecebido)
        {
            // Ctrl+C solicita EOS uma unica vez, na thread principal.
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

            // A espera de 100 ms permite consultar novamente o sinal e o prazo.
            // Um retorno nulo significa apenas que nenhuma mensagem chegou ainda.
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

                // Registrar qual elemento falhou ajuda a diferenciar erros de
                // negociacao, plugins ausentes e problemas do dispositivo.
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

            // O filtro admite apenas ERROR ou EOS. Se nao houve erro, terminou.
            eosRecebido = true;
            gst_message_unref(mensagem);
        }

        // 8. Parar as threads antes de consultar os dados compartilhados.
        // Assim, nenhum probe altera contadores durante a escrita do relatorio.
        gst_element_set_state(pipeline, GST_STATE_NULL);

        if (!original.temFormato || !processado.temFormato || original.falhou || processado.falhou)
        {
            throw std::runtime_error("Falha na medicao ou gravacao de imagem.");
        }

        // 9. Apresentar resultados reais no terminal e, opcionalmente, em JSON.
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

    // 10. Limpeza comum aos caminhos de sucesso e falha.
    // NULL interrompe a pipeline antes de liberar as referencias ao bus e a ela.
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
