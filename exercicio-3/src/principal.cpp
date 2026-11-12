#include <gst/gst.h>

#include <charconv>
#include <chrono>
#include <iostream>
#include <string>

struct Opcoes
{
    std::string modo;
    std::string destino = "127.0.0.1";
    unsigned porta = 5004;
    unsigned latencia = 200;
    unsigned duracao = 0;
    bool silencioso = false;
};

bool lerNumero(const std::string& texto, unsigned& valor, unsigned minimo, unsigned maximo)
{
    const auto resultado = std::from_chars(texto.data(), texto.data() + texto.size(), valor);
    return resultado.ec == std::errc() && resultado.ptr == texto.data() + texto.size() &&
           valor >= minimo && valor <= maximo;
}

bool lerOpcoes(int argc, char* argv[], Opcoes& opcoes)
{
    opcoes.modo = argv[1];
    if (opcoes.modo != "transmissor" && opcoes.modo != "receptor")
    {
        std::cerr << "Informe o modo transmissor ou receptor.\n";
        return false;
    }

    for (int i = 2; i < argc; ++i)
    {
        const std::string argumento = argv[i];
        if (argumento == "--silencioso" && opcoes.modo == "receptor")
        {
            opcoes.silencioso = true;
            continue;
        }

        if (i + 1 >= argc)
        {
            std::cerr << "Falta valor para " << argumento << '\n';
            return false;
        }

        const std::string valor = argv[++i];
        bool valido = false;

        if (argumento == "--destino" && opcoes.modo == "transmissor")
        {
            opcoes.destino = valor;
            valido = !valor.empty();
        }
        else if (argumento == "--porta")
            valido = lerNumero(valor, opcoes.porta, 1, 65535);
        else if (argumento == "--latencia" && opcoes.modo == "receptor")
            valido = lerNumero(valor, opcoes.latencia, 0, 60000);
        else if (argumento == "--duracao")
            valido = lerNumero(valor, opcoes.duracao, 0, 86400);

        if (!valido)
        {
            std::cerr << "Opcao ou valor invalido: " << argumento << " " << valor << '\n';
            return false;
        }
    }

    return true;
}

GstElement* criarPipeline(const Opcoes& opcoes)
{
    // O transmissor gera audio, codifica em PCMU e envia pacotes RTP por UDP.
    const char* transmissor =
        "audiotestsrc is-live=true wave=sine freq=440 ! "
        "audioconvert ! audioresample ! audio/x-raw,rate=8000,channels=1 ! "
        "mulawenc ! rtppcmupay pt=0 ! udpsink name=rede";

    // O receptor compensa jitter, decodifica o audio e escolhe a saida.
    const char* receptor =
        "udpsrc name=rede "
        "caps=\"application/x-rtp,media=(string)audio,clock-rate=(int)8000,"
        "encoding-name=(string)PCMU,payload=(int)0\" ! "
        "rtpjitterbuffer name=jitter ! rtppcmudepay ! mulawdec ! "
        "audioconvert ! audioresample ! ";

    const std::string descricao = opcoes.modo == "transmissor"
        ? transmissor
        : std::string(receptor) + (opcoes.silencioso ? "fakesink sync=true" : "autoaudiosink");

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
        g_object_set(rede, "host", opcoes.destino.c_str(),
                     "port", static_cast<int>(opcoes.porta), nullptr);
    }
    else
    {
        g_object_set(rede, "port", static_cast<int>(opcoes.porta), nullptr);
        jitter = gst_bin_get_by_name(GST_BIN(pipeline), "jitter");
        g_object_set(jitter, "latency", opcoes.latencia, "do-lost", TRUE, nullptr);
    }
    gst_object_unref(rede);

    std::cout << opcoes.modo << " porta=" << opcoes.porta;
    if (jitter)
        std::cout << " latencia=" << opcoes.latencia << "ms"
                  << (opcoes.silencioso ? " (sem audio)" : "");
    else
        std::cout << " destino=" << opcoes.destino;
    std::cout << std::endl;

    return jitter;
}

void mostrarEstatisticas(GstElement* jitter)
{
    if (!jitter)
        return;

    GstStructure* estatisticas = nullptr;
    g_object_get(jitter, "stats", &estatisticas, nullptr);
    if (estatisticas)
    {
        gchar* texto = gst_structure_to_string(estatisticas);
        std::cout << "Estatisticas: " << texto << std::endl;
        g_free(texto);
        gst_structure_free(estatisticas);
    }
}

int executarPipeline(GstElement* pipeline, unsigned duracao)
{
    GstBus* bus = gst_element_get_bus(pipeline);
    int resultado = 0;

    if (gst_element_set_state(pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE)
    {
        std::cerr << "Falha ao iniciar a pipeline.\n";
        resultado = 1;
    }
    else
    {
        const auto inicio = std::chrono::steady_clock::now();
        while (true)
        {
            GstMessage* mensagem = gst_bus_timed_pop_filtered(
                bus, GST_SECOND, static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

            if (mensagem)
            {
                if (GST_MESSAGE_TYPE(mensagem) == GST_MESSAGE_ERROR)
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
                }
                gst_message_unref(mensagem);
                break;
            }

            if (duracao && std::chrono::steady_clock::now() - inicio >= std::chrono::seconds(duracao))
                break;
        }
    }

    gst_object_unref(bus);
    return resultado;
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "Informe o modo transmissor ou receptor.\n";
        return 1;
    }

    Opcoes opcoes;
    if (!lerOpcoes(argc, argv, opcoes))
        return 1;

    gst_init(nullptr, nullptr);
    GstElement* pipeline = criarPipeline(opcoes);
    if (!pipeline)
        return 1;

    GstElement* jitter = configurarPipeline(pipeline, opcoes);
    const int resultado = executarPipeline(pipeline, opcoes.duracao);
    mostrarEstatisticas(jitter);

    if (jitter)
        gst_object_unref(jitter);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    return resultado;
}
