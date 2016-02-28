_Pragma("once")
namespace mixxx {

    class Logging {
    public:
        static void initialize();
        static void shutdown();

    private:
        Logging() = delete;
    };

    void install_message_handler();

}  // namespace mixxx
