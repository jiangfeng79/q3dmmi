#ifndef WORKERENTRY_H
#define WORKERENTRY_H

#include <QThread>

#include <memory>
#include <vector>

class WorkerEntry
{
public:
    explicit WorkerEntry(QObject* parent) : m_thread(new QThread(parent)) {}
    virtual ~WorkerEntry() = default;

    QThread* thread() const { return m_thread; }
    virtual QObject* worker() const = 0;
    virtual void stop() = 0;

private:
    QThread* m_thread;
};

template <typename Worker>
class WorkerEntryImpl final : public WorkerEntry
{
public:
    explicit WorkerEntryImpl(QObject* parent) : WorkerEntry(parent), m_worker(new Worker)
    {
        m_worker->moveToThread(thread());
    }

    ~WorkerEntryImpl() override { delete m_worker; }

    Worker* typedWorker() const { return m_worker; }
    QObject* worker() const override { return m_worker; }
    void stop() override { QMetaObject::invokeMethod(m_worker, &Worker::stop, Qt::QueuedConnection); }

private:
    Worker* m_worker;
};

enum WorkerIndex
{
    FlightWorkerIndex,
    BusWorkerIndex,
    BusRouteWorkerIndex
};

template <typename Worker>
Worker* workerAt(const std::vector<std::unique_ptr<WorkerEntry>>& workers, WorkerIndex index)
{
    return static_cast<WorkerEntryImpl<Worker>*>(workers[index].get())->typedWorker();
}

#endif  // WORKERENTRY_H