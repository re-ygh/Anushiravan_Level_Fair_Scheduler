struct Task {
    int id;
    int nice;
    double vruntime;
    double weight;
    bool runnable;
    std::set<int> affinity;
    int cgroup_id;
};
