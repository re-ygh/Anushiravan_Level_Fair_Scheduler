int main() {
    Config cfg;
    Scheduler scheduler(cfg.cpus, cfg.quantum);
    UnixSocket socket;

    socket.connect(cfg.socket_path);

    while (true) {
        auto json = socket.receive();
        auto events = parse_events(json);
        scheduler.tick(events.vtime);
    }
}
