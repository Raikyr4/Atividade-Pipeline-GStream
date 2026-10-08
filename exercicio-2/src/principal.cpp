#include <gst/gst.h>

#include <filesystem>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

struct ConfiguracaoPcm
{
    const char* nome;
    const char* caps;
    const char* descricao;
};

int main(int argc, char* argv[])
{
    const ConfiguracaoPcm configuracaoA{"A", "audio/x-raw,format=S16LE,rate=44100,channels=2", "44.100 Hz, 16 bits, estereo"};
    const ConfiguracaoPcm configuracaoB{"B", "audio/x-raw,format=S16LE,rate=8000,channels=1", "8.000 Hz, 16 bits, mono"};

    const ConfiguracaoPcm* pcm = &configuracaoA;
    
    fs::path saida = "saida_pcm_A.mov";
    fs::path video;

    for (int i = 1; i < argc; ++i)
    {
        const std::string argumento = argv[i];

        if (argumento == "--ajuda")
        {
            std::cout << "Uso: exercicio_2 --video entrada [--pcm A|B] [--saida arquivo.mov]\n"
                         "A: 44100 Hz, S16LE, 2 canais\n"
                         "B: 8000 Hz, S16LE, 1 canal\n";
            return 0;
        }
        if (argumento == "--pcm" && i + 1 < argc)
        {
            const std::string valor = argv[++i];
            if (valor == "A" || valor == "a")
                pcm = &configuracaoA;
            else if (valor == "B" || valor == "b")
                pcm = &configuracaoB;
            else
            {
                std::cerr << "Erro: PCM deve ser A ou B.\n";
                return 1;
            }
        }
        else if (argumento == "--video" && i + 1 < argc)
        {
            video = argv[++i];
        }
        else if (argumento == "--saida" && i + 1 < argc)
        {
            saida = argv[++i];
        }
        else
        {
            std::cerr << "Erro: argumento invalido: " << argumento << '\n';
            return 1;
        }
    }

    // Se a configuracao foi escolhida e a saida nao foi informada, o nome acompanha a escolha.
    if (saida == "saida_pcm_A.mov" && pcm == &configuracaoB)
        saida = "saida_pcm_B.mov";

    if (video.empty() || !fs::is_regular_file(video))
    {
        std::cerr << "Erro: informe um video existente com --video. Recebido: " << video << '\n';
        return 1;
    }

    if (fs::exists(saida))
    {
        std::cerr << "Erro: arquivo de saida ja existe: " << saida << '\n';
        return 1;
    }

    gst_init(&argc, &argv);

    // Video existente: decodebin identifica container e codecs e cria um pad por fluxo.
    // Os pads sao dinamicos; gst_parse_launch liga cada um ao primeiro elemento compativel
    // (videoconvert aceita apenas video, audioconvert apenas audio).
    // Video: reencodado em H.264. I420 gera perfil High (4:2:0), compativel com a maioria dos players.
    // Audio: audioconvert e audioresample convertem formato/canais e taxa para as caps A ou B.
    const std::string pipelineTexto =
        "qtmux name=mux ! filesink name=arquivo "
        "filesrc name=entrada ! decodebin name=dec "
        "dec. ! videoconvert ! video/x-raw,format=I420 ! "
        "x264enc tune=zerolatency ! h264parse ! queue ! mux. "
        "dec. ! audioconvert ! audioresample ! " +
        std::string(pcm->caps) + " ! queue ! mux.";

    GError* erro = nullptr;
    GstElement* pipeline = gst_parse_launch(pipelineTexto.c_str(), &erro);

    if (!pipeline || erro)
    {
        std::cerr << "Erro ao criar pipeline: " << (erro ? erro->message : "desconhecido") << '\n';
        g_clear_error(&erro);
        return 1;
    }

    GstElement* entrada = gst_bin_get_by_name(GST_BIN(pipeline), "entrada");
    g_object_set(entrada, "location", video.string().c_str(), nullptr);
    gst_object_unref(entrada);

    GstElement* arquivo = gst_bin_get_by_name(GST_BIN(pipeline), "arquivo");
    g_object_set(arquivo, "location", saida.string().c_str(), nullptr);
    gst_object_unref(arquivo);

    std::cout << "Entrada: " << video.string() << "\nGerando " << saida << "\nPCM " << pcm->nome << ": " << pcm->descricao
              << "\nVideo: H.264\n";

    GstBus* bus = gst_element_get_bus(pipeline);
    int resultado = 0;

    if (gst_element_set_state(pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE)
    {
        std::cerr << "Erro ao iniciar a pipeline. Verifique os plugins x264 e isomp4.\n";
        resultado = 1;
    }
    else
    {
        GstMessage* mensagem = gst_bus_timed_pop_filtered(
            bus, GST_CLOCK_TIME_NONE,
            static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

        if (GST_MESSAGE_TYPE(mensagem) == GST_MESSAGE_ERROR)
        {
            GError* falha = nullptr;
            gchar* detalhes = nullptr;
            gst_message_parse_error(mensagem, &falha, &detalhes);
            std::cerr << "Erro na pipeline: " << falha->message << '\n';
            g_clear_error(&falha);
            g_free(detalhes);
            resultado = 1;
        }
        else
        {
            std::cout << "Concluido: " << saida << '\n';
        }
        gst_message_unref(mensagem);
    }

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(bus);
    gst_object_unref(pipeline);
    return resultado;
}
