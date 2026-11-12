#include <gst/gst.h>
#include <charconv>
#include <chrono>
#include <iostream>
#include <string>

struct Opcoes {
    std::string modo;
    std::string destino = "127.0.0.1";
    unsigned porta = 5004, latencia = 200, duracao = 0;
    bool silencioso = false;
};

bool numero(const std::string& texto, unsigned& valor, unsigned minimo, unsigned maximo) {
    auto r = std::from_chars(texto.data(), texto.data() + texto.size(), valor);
    return r.ec == std::errc() && r.ptr == texto.data() + texto.size() && valor >= minimo && valor <= maximo;
}

void ajuda() {
    std::cout << "Uso: exercicio_3 transmissor|receptor [opcoes]\n"
              << "  --destino ENDERECO  destino do transmissor (padrao 127.0.0.1)\n"
              << "  --porta NUMERO      porta UDP (padrao 5004)\n"
              << "  --latencia MS       buffer do receptor (padrao 200)\n"
              << "  --duracao SEGUNDOS  encerra apos o tempo indicado; 0 = continuo\n"
              << "  --silencioso        receptor sem dispositivo de audio, para testes\n";
}

int main(int argc, char* argv[]) {
    Opcoes o;
    if (argc < 2 || std::string(argv[1]) == "--ajuda") { ajuda(); return argc < 2 ? 1 : 0; }
    o.modo = argv[1];
    if (o.modo != "transmissor" && o.modo != "receptor") { ajuda(); return 1; }
    for (int i = 2; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--silencioso" && o.modo == "receptor") { o.silencioso = true; continue; }
        if (i + 1 >= argc) { std::cerr << "Falta valor para " << a << '\n'; return 1; }
        std::string v = argv[++i];
        bool valido = false;
        if (a == "--destino" && o.modo == "transmissor") { o.destino = v; valido = !v.empty(); }
        else if (a == "--porta") valido = numero(v, o.porta, 1, 65535);
        else if (a == "--latencia" && o.modo == "receptor") valido = numero(v, o.latencia, 0, 60000);
        else if (a == "--duracao") valido = numero(v, o.duracao, 0, 86400);
        if (!valido) { std::cerr << "Opcao ou valor invalido: " << a << " " << v << '\n'; return 1; }
    }
    gst_init(nullptr, nullptr);
    const char* tx = "audiotestsrc is-live=true wave=sine freq=440 ! audioconvert ! audioresample ! "
                     "audio/x-raw,rate=8000,channels=1 ! mulawenc ! rtppcmupay pt=0 ! udpsink name=rede";
    const char* rx = "udpsrc name=rede caps=\"application/x-rtp,media=(string)audio,clock-rate=(int)8000,encoding-name=(string)PCMU,payload=(int)0\" "
                     "! rtpjitterbuffer name=jitter ! rtppcmudepay ! mulawdec ! audioconvert ! audioresample ! ";
    std::string descricao = o.modo == "transmissor" ? tx : std::string(rx) + (o.silencioso ? "fakesink sync=true" : "autoaudiosink");
    GError* erro = nullptr;
    GstElement* pipeline = gst_parse_launch(descricao.c_str(), &erro);
    if (!pipeline || erro) {
        std::cerr << "Erro ao criar pipeline: " << (erro ? erro->message : "desconhecido") << '\n';
        g_clear_error(&erro); if (pipeline) gst_object_unref(pipeline); return 1;
    }
    GstElement* rede = gst_bin_get_by_name(GST_BIN(pipeline), "rede");
    GstElement* jitter = nullptr;
    if (o.modo == "transmissor") g_object_set(rede, "host", o.destino.c_str(), "port", static_cast<int>(o.porta), nullptr);
    else {
        g_object_set(rede, "port", static_cast<int>(o.porta), nullptr);
        jitter = gst_bin_get_by_name(GST_BIN(pipeline), "jitter");
        g_object_set(jitter, "latency", o.latencia, "do-lost", TRUE, nullptr);
    }
    gst_object_unref(rede);
    std::cout << o.modo << " porta=" << o.porta;
    if (jitter) std::cout << " latencia=" << o.latencia << "ms" << (o.silencioso ? " (sem audio)" : "");
    else std::cout << " destino=" << o.destino;
    std::cout << std::endl;
    GstBus* bus = gst_element_get_bus(pipeline);
    int resultado = 0;
    if (gst_element_set_state(pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
        std::cerr << "Falha ao iniciar a pipeline.\n"; resultado = 1;
    } else {
        auto inicio = std::chrono::steady_clock::now();
        while (true) {
            GstMessage* msg = gst_bus_timed_pop_filtered(bus, GST_SECOND,
                static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
            if (msg) {
                if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
                    GError* falha = nullptr; gchar* detalhes = nullptr;
                    gst_message_parse_error(msg, &falha, &detalhes);
                    std::cerr << "Erro: " << (falha ? falha->message : "desconhecido") << '\n';
                    if (detalhes) std::cerr << detalhes << '\n';
                    g_clear_error(&falha); g_free(detalhes); resultado = 1;
                }
                gst_message_unref(msg); break;
            }
            if (o.duracao && std::chrono::steady_clock::now() - inicio >= std::chrono::seconds(o.duracao)) break;
        }
    }
    if (jitter) {
        GstStructure* stats = nullptr;
        g_object_get(jitter, "stats", &stats, nullptr);
        if (stats) { gchar* texto = gst_structure_to_string(stats); std::cout << "Estatisticas: " << texto << std::endl; g_free(texto); gst_structure_free(stats); }
        gst_object_unref(jitter);
    }
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(bus); gst_object_unref(pipeline);
    return resultado;
}
