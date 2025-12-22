class UnixSocket {
public:
    void connect(const std::string& path);
    std::string receive();
};
