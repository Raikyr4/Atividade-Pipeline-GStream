// Exercicio 4 - streaming de audio RTP/UDP entre dois dispositivos.
// Evolucao do exercicio 3: o mesmo transmissor/receptor PCMU, agora sem endereco fixo no codigo
// (IP e porta por argumento ou arquivo .ini), com estatisticas do jitter buffer a cada segundo,
// registro em CSV e simulacao opcional de rede ruim para comparar latencias.

#include <gst/gst.h>

#include <atomic>
#include <charconv>
#include <chrono>
#include <csignal>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace
{
struct Opcoes
{
    std::string modo;
    std::string destino; // sem valor padrao: precisa vir do argumento ou do .ini
    unsigned porta = 5004;
    unsigned latencia = 200;
    unsigned duracao = 0;
    bool silencioso = false;
    std::string fonte = "seno";
    std::string arquivo;
    bool simularRede = false;
    std::string csv;
};

std::atomic<bool> pedidoParada{false};

void tratarSinal(int)
{
    pedidoParada = true;
}

void mostrarAjuda()
{
    std::cout
        << "Uso:\n"
           "  exercicio_4 transmissor --destino IP [--porta N] [--fonte seno|bipe|mic|arquivo]\n"
           "                          [--arquivo CAMINHO] [--duracao S] [--simular-rede] [--config ARQ.ini]\n"
           "  exercicio_4 receptor [--porta N] [--latencia MS] [--duracao S] [--csv ARQ.csv]\n"
           "                       [--silencioso] [--config ARQ.ini]\n\n"
           "  --destino       IPv4 da maquina receptora (obrigatorio no transmissor)\n"
           "  --porta         porta UDP (padrao 5004)\n"
           "  --latencia      margem do rtpjitterbuffer em ms (padrao 200)\n"
           "  --fonte         seno 440 Hz (padrao), bipe a cada 1 s, microfone ou arquivo de midia\n"
           "  --simular-rede  netsim: atrasa 30% dos pacotes de 0 a 120 ms e descarta 1%\n"
           "  --csv           receptor grava as estatisticas de cada segundo\n"
           "  --config        le [transmissor] ou [receptor] de um .ini; argumentos tem prioridade\n";
}

bool lerNumero(const std::string& texto, unsigned& valor, unsigned minimo, unsigned maximo)
{
    unsigned lido = 0;
    const auto resultado = std::from_chars(texto.data(), texto.data() + texto.size(), lido);
    if (resultado.ec != std::errc() || resultado.ptr != texto.data() + texto.size() || lido < minimo ||
        lido > maximo)
        return false;
    valor = lido;
    return true;
}

// Aceita apenas IPv4 no formato a.b.c.d, cada parte entre 0 e 255.
bool ipv4Valido(const std::string& texto)
{
    if (texto.empty() || texto.back() == '.')
        return false;
    std::istringstream entrada(texto);
    std::string parte;
    int partes = 0;
    unsigned valor = 0;
    while (std::getline(entrada, parte, '.'))
    {
        if (parte.empty() || parte.size() > 3 || !lerNumero(parte, valor, 0, 255))
            return false;
        ++partes;
    }
    return partes == 4;
}

// Valores do .ini: viram a base, e os argumentos da linha de comando sobrescrevem.
bool lerConfiguracao(const std::string& caminho, Opcoes& opcoes)
{
    GKeyFile* ini = g_key_file_new();
    GError* erro = nullptr;
    if (!g_key_file_load_from_file(ini, caminho.c_str(), G_KEY_FILE_NONE, &erro))
    {
        std::cerr << "Erro ao ler " << caminho << ": " << erro->message << '\n';
        g_clear_error(&erro);
        g_key_file_free(ini);
        return false;
    }

    const char* grupo = opcoes.modo.c_str();
    bool valido = true;
    auto texto = [&](const char* chave, std::string& destino) {
        if (gchar* valor = g_key_file_get_string(ini, grupo, chave, nullptr))
        {
            destino = g_strstrip(valor);
            g_free(valor);
        }
    };
    auto numero = [&](const char* chave, unsigned& destino, unsigned minimo, unsigned maximo) {
        std::string valor;
        texto(chave, valor);
        if (!valor.empty() && !lerNumero(valor, destino, minimo, maximo))
        {
            std::cerr << "Valor invalido no .ini: " << chave << "=" << valor << '\n';
            valido = false;
        }
    };

    numero("porta", opcoes.porta, 1, 65535);
    if (opcoes.modo == "transmissor")
    {
        texto("destino", opcoes.destino);
        texto("fonte", opcoes.fonte);
        texto("arquivo", opcoes.arquivo);
    }
    else
        numero("latencia", opcoes.latencia, 0, 60000);

    g_key_file_free(ini);
    return valido;
}

bool lerOpcoes(int argc, char* argv[], Opcoes& opcoes)
{
    opcoes.modo = argv[1];
    if (opcoes.modo == "--ajuda" || opcoes.modo == "-h")
    {
        mostrarAjuda();
        return false;
    }
    if (opcoes.modo != "transmissor" && opcoes.modo != "receptor")
    {
        std::cerr << "Informe o modo transmissor ou receptor (use --ajuda).\n";
        return false;
    }

    // 1a passada: arquivo de configuracao.
    for (int i = 2; i + 1 < argc; ++i)
        if (std::string(argv[i]) == "--config" && !lerConfiguracao(argv[i + 1], opcoes))
            return false;

    // 2a passada: argumentos.
    const bool transmissor = opcoes.modo == "transmissor";
    for (int i = 2; i < argc; ++i)
    {
        const std::string argumento = argv[i];
        if (argumento == "--silencioso" && !transmissor)
        {
            opcoes.silencioso = true;
            continue;
        }
        if (argumento == "--simular-rede" && transmissor)
        {
            opcoes.simularRede = true;
            continue;
        }
        if (i + 1 >= argc)
        {
            std::cerr << "Falta valor para " << argumento << '\n';
            return false;
        }

        const std::string valor = argv[++i];
        bool valido = false;

        if (argumento == "--config")
            valido = true; // ja lido na 1a passada
        else if (argumento == "--destino" && transmissor)
        {
            opcoes.destino = valor;
            valido = true;
        }
        else if (argumento == "--porta")
            valido = lerNumero(valor, opcoes.porta, 1, 65535);
        else if (argumento == "--latencia" && !transmissor)
            valido = lerNumero(valor, opcoes.latencia, 0, 60000);
        else if (argumento == "--duracao")
            valido = lerNumero(valor, opcoes.duracao, 0, 86400);
        else if (argumento == "--fonte" && transmissor)
        {
            opcoes.fonte = valor;
            valido = true;
        }
        else if (argumento == "--arquivo" && transmissor)
        {
            opcoes.arquivo = valor;
            valido = true;
        }
        else if (argumento == "--csv" && !transmissor)
        {
            opcoes.csv = valor;
            valido = !valor.empty();
        }

        if (!valido)
        {
            std::cerr << "Opcao ou valor invalido: " << argumento << " " << valor << '\n';
            return false;
        }
    }

    if (transmissor)
    {
        if (opcoes.destino.empty())
        {
            std::cerr << "Informe o IP da maquina receptora com --destino (ou destino= no .ini).\n";
            return false;
        }
        if (!ipv4Valido(opcoes.destino))
        {
            std::cerr << "Destino nao e um IPv4 valido (ex.: 192.168.10.2): " << opcoes.destino << '\n';
            return false;
        }
        if (opcoes.fonte != "seno" && opcoes.fonte != "bipe" && opcoes.fonte != "mic" && opcoes.fonte != "arquivo")
        {
            std::cerr << "Fonte deve ser seno, bipe, mic ou arquivo.\n";
            return false;
        }
        if (opcoes.fonte == "arquivo" && !g_file_test(opcoes.arquivo.c_str(), G_FILE_TEST_IS_REGULAR))
        {
            std::cerr << "--fonte arquivo exige --arquivo existente. Recebido: " << opcoes.arquivo << '\n';
            return false;
        }
    }
    return true;
}

std::string descreverPipeline(const Opcoes& opcoes)
{
    if (opcoes.modo == "transmissor")
    {
        std::string fonte;
        if (opcoes.fonte == "seno")
            fonte = "audiotestsrc is-live=true wave=sine freq=440";
        else if (opcoes.fonte == "bipe") // bipe curto a cada 1 s: facilita perceber o atraso
            fonte = "audiotestsrc is-live=true wave=ticks";
        else if (opcoes.fonte == "mic")
            fonte = "autoaudiosrc";
        else
            fonte = "filesrc name=entrada ! decodebin";

        // netsim imita Wi-Fi congestionado para comparar as latencias do jitter buffer.
        const std::string rede = opcoes.simularRede ? "netsim delay-probability=0.3 min-delay=0 max-delay=120 "
                                                      "drop-probability=0.01 allow-reordering=true ! "
                                                    : "";

        // O transmissor codifica em PCMU (G.711 mu-law, 8 kHz mono) e envia pacotes RTP por UDP.
        // max-ptime = 20 ms por pacote (160 amostras, padrao de VoIP). No exercicio 3 cada pacote
        // levava um buffer inteiro do audiotestsrc (1024 amostras = 128 ms), atraso grande para tempo real.
        return fonte + " ! audioconvert ! audioresample ! audio/x-raw,rate=8000,channels=1 ! "
                       "mulawenc ! rtppcmupay pt=0 max-ptime=20000000 ! " +
               rede + "udpsink name=rede sync=true";
    }

    // O receptor declara as caps RTP (UDP nao informa o formato), compensa jitter e reproduz.
    // timeout: o udpsrc avisa no bus se passar 3 s sem pacotes.
    return std::string("udpsrc name=rede timeout=3000000000 "
                       "caps=\"application/x-rtp,media=(string)audio,clock-rate=(int)8000,"
                       "encoding-name=(string)PCMU,payload=(int)0\" ! "
                       "rtpjitterbuffer name=jitter ! rtppcmudepay ! mulawdec ! "
                       "audioconvert ! audioresample ! ") +
           (opcoes.silencioso ? "fakesink sync=true" : "autoaudiosink");
}

GstElement* criarPipeline(const std::string& descricao)
{
    GError* erro = nullptr;
    GstElement* pipeline = gst_parse_launch(descricao.c_str(), &erro);
    if (!pipeline || erro)
    {
        std::cerr << "Erro ao criar pipeline: " << (erro ? erro->message : "desconhecido") << '\n';
        g_clear_error(&erro);
        if (pipeline)
            gst_object_unref(pipeline);
        return nullptr;
    }
    return pipeline;
}

GstElement* configurarPipeline(GstElement* pipeline, const Opcoes& opcoes)
{
    GstElement* rede = gst_bin_get_by_name(GST_BIN(pipeline), "rede");
    GstElement* jitter = nullptr;

    if (opcoes.modo == "transmissor")
    {
        g_object_set(rede, "host", opcoes.destino.c_str(), "port", static_cast<int>(opcoes.porta), nullptr);
        if (opcoes.fonte == "arquivo")
        {
            GstElement* entrada = gst_bin_get_by_name(GST_BIN(pipeline), "entrada");
            g_object_set(entrada, "location", opcoes.arquivo.c_str(), nullptr);
            gst_object_unref(entrada);
        }
        std::cout << "transmissor destino=" << opcoes.destino << ":" << opcoes.porta << " fonte=" << opcoes.fonte
                  << (opcoes.simularRede ? " rede=SIMULADA" : " rede=real") << std::endl;
    }
    else
    {
        g_object_set(rede, "port", static_cast<int>(opcoes.porta), nullptr);
        jitter = gst_bin_get_by_name(GST_BIN(pipeline), "jitter");
        g_object_set(jitter, "latency", opcoes.latencia, "do-lost", TRUE, nullptr);
        std::cout << "receptor escutando 0.0.0.0:" << opcoes.porta << " latencia=" << opcoes.latencia << "ms"
                  << (opcoes.silencioso ? " (sem audio)" : "") << std::endl;
    }
    gst_object_unref(rede);
    return jitter;
}

struct Estatisticas
{
    guint64 recebidos = 0, perdidos = 0, atrasados = 0, duplicados = 0, jitterNs = 0;
};

Estatisticas lerEstatisticas(GstElement* jitter)
{
    Estatisticas e;
    GstStructure* s = nullptr;
    g_object_get(jitter, "stats", &s, nullptr);
    if (s)
    {
        gst_structure_get_uint64(s, "num-pushed", &e.recebidos);
        gst_structure_get_uint64(s, "num-lost", &e.perdidos);
        gst_structure_get_uint64(s, "num-late", &e.atrasados);
        gst_structure_get_uint64(s, "num-duplicates", &e.duplicados);
        gst_structure_get_uint64(s, "avg-jitter", &e.jitterNs);
        gst_structure_free(s);
    }
    return e;
}

void mostrarResumo(GstElement* pipeline, GstElement* jitter, unsigned latencia)
{
    const Estatisticas e = lerEstatisticas(jitter);
    const guint64 esperados = e.recebidos + e.perdidos;
    const double perdaPct = esperados ? 100.0 * static_cast<double>(e.perdidos) / static_cast<double>(esperados) : 0.0;

    GstQuery* consulta = gst_query_new_latency();
    GstClockTime latMin = 0;
    const bool temLatencia = gst_element_query(pipeline, consulta);
    if (temLatencia)
        gst_query_parse_latency(consulta, nullptr, &latMin, nullptr);
    gst_query_unref(consulta);

    std::cout << std::fixed << std::setprecision(2) << "\nResumo (latencia do jitter buffer = " << latencia << " ms)\n"
              << "  num-pushed (tocados)     : " << e.recebidos << '\n'
              << "  num-lost (perdidos)      : " << e.perdidos << " (" << perdaPct << " %)\n"
              << "  num-late (atrasados)     : " << e.atrasados << '\n'
              << "  num-duplicates           : " << e.duplicados << '\n'
              << "  avg-jitter               : " << static_cast<double>(e.jitterNs) / 1e6 << " ms\n";
    if (temLatencia)
        std::cout << "  latencia da pipeline     : " << static_cast<double>(latMin) / 1e6 << " ms\n";
    if (e.recebidos == 0)
        std::cout << "  ATENCAO: nenhum pacote recebido.\n";
}

int executarPipeline(GstElement* pipeline, GstElement* jitter, const Opcoes& opcoes)
{
    std::ofstream csv;
    if (jitter && !opcoes.csv.empty())
    {
        csv.open(opcoes.csv);
        if (!csv)
        {
            std::cerr << "Nao foi possivel criar " << opcoes.csv << '\n';
            return 1;
        }
        csv << "segundo,latencia_ms,num_pushed,num_lost,num_late,num_duplicates,avg_jitter_ms\n";
    }

    GstBus* bus = gst_element_get_bus(pipeline);
    int resultado = 0;

    if (gst_element_set_state(pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE)
    {
        std::cerr << "Falha ao iniciar a pipeline (porta em uso ou dispositivo de audio indisponivel).\n";
        gst_object_unref(bus);
        return 1;
    }

    if (jitter)
        std::cout << "tempo | num-pushed | num-lost | num-late | avg-jitter" << std::endl;

    const auto inicio = std::chrono::steady_clock::now();
    unsigned ultimoSegundo = 0;
    while (!pedidoParada)
    {
        GstMessage* mensagem = gst_bus_timed_pop_filtered(
            bus, 200 * GST_MSECOND,
            static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS | GST_MESSAGE_ELEMENT));

        if (mensagem)
        {
            const GstStructure* s = gst_message_get_structure(mensagem);
            const GstMessageType tipo = GST_MESSAGE_TYPE(mensagem);
            bool sair = false;
            if (tipo == GST_MESSAGE_ELEMENT && s && gst_structure_has_name(s, "GstUDPSrcTimeout"))
                std::cout << "  ... 3 s sem pacotes na porta " << opcoes.porta
                          << " (confira IP/porta do transmissor e o firewall)" << std::endl;
            else if (tipo == GST_MESSAGE_ERROR)
            {
                GError* falha = nullptr;
                gchar* detalhes = nullptr;
                gst_message_parse_error(mensagem, &falha, &detalhes);
                std::cerr << "Erro: " << (falha ? falha->message : "desconhecido") << '\n';
                if (detalhes)
                    std::cerr << detalhes << '\n';
                g_clear_error(&falha);
                g_free(detalhes);
                resultado = 1;
                sair = true;
            }
            else if (tipo == GST_MESSAGE_EOS)
            {
                std::cout << "Fim do fluxo." << std::endl;
                sair = true;
            }
            gst_message_unref(mensagem);
            if (sair)
                break;
        }

        const auto segundo = static_cast<unsigned>(
            std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - inicio).count());
        if (segundo > ultimoSegundo)
        {
            ultimoSegundo = segundo;
            if (jitter)
            {
                const Estatisticas e = lerEstatisticas(jitter);
                const double jitterMs = static_cast<double>(e.jitterNs) / 1e6;
                std::cout << std::setw(4) << segundo << "s | " << std::setw(10) << e.recebidos << " | " << std::setw(8)
                          << e.perdidos << " | " << std::setw(8) << e.atrasados << " | " << std::fixed
                          << std::setprecision(2) << jitterMs << " ms" << std::endl;
                if (csv)
                    csv << segundo << ',' << opcoes.latencia << ',' << e.recebidos << ',' << e.perdidos << ','
                        << e.atrasados << ',' << e.duplicados << ',' << jitterMs << '\n';
            }
            if (opcoes.duracao && segundo >= opcoes.duracao)
                break;
        }
    }

    if (jitter && resultado == 0)
        mostrarResumo(pipeline, jitter, opcoes.latencia);

    gst_object_unref(bus);
    return resultado;
}
} // namespace

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        mostrarAjuda();
        return 1;
    }

    Opcoes opcoes;
    if (!lerOpcoes(argc, argv, opcoes))
        return (opcoes.modo == "--ajuda" || opcoes.modo == "-h") ? 0 : 1;

    gst_init(nullptr, nullptr);
    const std::string descricao = descreverPipeline(opcoes);
    GstElement* pipeline = criarPipeline(descricao);
    if (!pipeline)
        return 1;
    std::cout << "pipeline: " << descricao << std::endl;

    // Ctrl+C so marca a parada: o laco encerra e o receptor ainda imprime o resumo.
    std::signal(SIGINT, tratarSinal);

    GstElement* jitter = configurarPipeline(pipeline, opcoes);
    const int resultado = executarPipeline(pipeline, jitter, opcoes);

    if (jitter)
        gst_object_unref(jitter);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    return resultado;
}
