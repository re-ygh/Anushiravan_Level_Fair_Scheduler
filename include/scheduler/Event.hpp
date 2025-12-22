enum class EventType {
    TASK_CREATE,
    TASK_BLOCK,
    TASK_UNBLOCK,
    TASK_EXIT,
    TASK_YIELD,
    TASK_SETNICE,
    TASK_SET_AFFINITY
};

struct Event {
    EventType type;
    int task_id;
    int value;
};
