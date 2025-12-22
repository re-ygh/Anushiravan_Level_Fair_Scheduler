class Scheduler {
public:
    Scheduler(int cpu_count, int quantum);
    void handle_event(const Event&);
    void tick(int vtime);
};
