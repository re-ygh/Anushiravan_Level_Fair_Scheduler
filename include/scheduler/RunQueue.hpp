class RunQueue {
public:
    void push(Task*);
    Task* pop();
    void remove(Task*);
    bool empty() const;
};
