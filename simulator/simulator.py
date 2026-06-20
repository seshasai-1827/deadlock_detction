import random
import csv
import numpy as np
from collections import deque

NUM_SAMPLES = 30000

# =====================================================
# RTOS Objects
# =====================================================

class Task:

    def __init__(self, tid):
        self.id = tid
        self.waiting_for = None
        self.wait_time = 0
        self.held_mutexes = set()

class Mutex:

    def __init__(self, mid):
        self.id = mid
        self.owner = None
        self.wait_queue = deque()

# =====================================================
# Simulator
# =====================================================

class RTOSSimulator:

    def __init__(self, n_tasks, n_mutexes):

        self.tasks = [Task(i) for i in range(n_tasks)]
        self.mutexes = [Mutex(i) for i in range(n_mutexes)]

        self.acquire_attempts = 0
        self.acquire_failures = 0

    def acquire(self, task, mutex):

        self.acquire_attempts += 1

        if mutex.owner is None:

            mutex.owner = task
            task.held_mutexes.add(mutex)

        else:

            self.acquire_failures += 1

            task.waiting_for = mutex
            mutex.wait_queue.append(task)

            task.wait_time += random.randint(1, 50)

    def release(self, task):

        if not task.held_mutexes:
            return

        mutex = random.choice(list(task.held_mutexes))

        task.held_mutexes.remove(mutex)

        if mutex.wait_queue:

            nxt = mutex.wait_queue.popleft()

            mutex.owner = nxt
            nxt.waiting_for = None

            nxt.held_mutexes.add(mutex)

        else:

            mutex.owner = None

    def timeout(self, task):

        if task.waiting_for:

            try:
                task.waiting_for.wait_queue.remove(task)
            except:
                pass

            task.waiting_for = None

    def random_event(self):

        task = random.choice(self.tasks)

        event = random.choice(
            ["acquire", "release", "timeout"]
        )

        if event == "acquire":

            mutex = random.choice(self.mutexes)

            self.acquire(task, mutex)

        elif event == "release":

            self.release(task)

        else:

            self.timeout(task)

    def inject_deadlock(self):

        if len(self.tasks) < 2:
            return

        if len(self.mutexes) < 2:
            return

        t1, t2 = random.sample(self.tasks, 2)
        m1, m2 = random.sample(self.mutexes, 2)

        m1.owner = t1
        m2.owner = t2

        t1.held_mutexes.add(m1)
        t2.held_mutexes.add(m2)

        t1.waiting_for = m2
        t2.waiting_for = m1

        m2.wait_queue.append(t1)
        m1.wait_queue.append(t2)

# =====================================================
# Graph Utilities
# =====================================================

def build_wait_graph(tasks):

    graph = {}

    for task in tasks:

        if task.waiting_for:

            owner = task.waiting_for.owner

            if owner:
                graph.setdefault(task.id, []).append(owner.id)

    return graph

def cycle_exists(graph):

    visited = set()
    stack = set()

    def dfs(v):

        visited.add(v)
        stack.add(v)

        for u in graph.get(v, []):

            if u not in visited:

                if dfs(u):
                    return True

            elif u in stack:
                return True

        stack.remove(v)

        return False

    for node in graph:

        if node not in visited:

            if dfs(node):
                return True

    return False

# cycle-safe depth calculation
def dependency_depth(graph):

    memo = {}

    def dfs(node, visiting):

        if node in memo:
            return memo[node]

        if node in visiting:
            return 0

        visiting.add(node)

        best = 0

        for nxt in graph.get(node, []):

            best = max(
                best,
                1 + dfs(nxt, visiting)
            )

        visiting.remove(node)

        memo[node] = best

        return best

    depth = 0

    for node in graph:

        depth = max(
            depth,
            dfs(node, set())
        )

    return depth

# =====================================================
# Feature Extraction
# =====================================================

def extract_features(sim):

    tasks = sim.tasks
    mutexes = sim.mutexes

    blocked_tasks = sum(
        1 for t in tasks
        if t.waiting_for
    )

    blocked_ratio = (
        blocked_tasks / len(tasks)
    )

    wait_times = [
        t.wait_time for t in tasks
    ]

    queue_lengths = [
        len(m.wait_queue)
        for m in mutexes
    ]

    graph = build_wait_graph(tasks)

    edges = sum(
        len(v)
        for v in graph.values()
    )

    density = edges / max(
        1,
        len(tasks) * (len(tasks)-1)
    )

    potential_cycle_count = 0

    for t in tasks:

        if t.waiting_for:

            owner = t.waiting_for.owner

            if owner and owner.waiting_for:
                potential_cycle_count += 1

    dep_depth = dependency_depth(graph)

    contention_rate = (
        sim.acquire_failures /
        max(1, sim.acquire_attempts)
    )

    mutexes_in_use = sum(
        1 for m in mutexes
        if m.owner
    )

    utilization = (
        mutexes_in_use /
        len(mutexes)
    )

    deadlock = cycle_exists(graph)

    score = (
        0.30 * blocked_ratio +
        0.20 * density +
        0.20 * min(dep_depth / 10, 1.0) +
        0.15 * contention_rate +
        0.10 * utilization +
        0.05 * min(potential_cycle_count / 5, 1.0)
    )

    score = min(score, 1.0)

    return [
        len(tasks),
        len(mutexes),
        blocked_tasks,
        blocked_ratio,
        np.mean(wait_times),
        np.max(wait_times),
        np.mean(queue_lengths),
        np.max(queue_lengths),
        density,
        potential_cycle_count,
        dep_depth,
        contention_rate,
        utilization,
        int(deadlock),
        score
    ]

# =====================================================
# Generate CSV
# =====================================================

header = [
    "num_tasks",
    "num_mutexes",
    "blocked_tasks",
    "blocked_ratio",
    "avg_wait_time",
    "max_wait_time",
    "avg_queue_length",
    "max_queue_length",
    "graph_density",
    "potential_cycle_count",
    "dependency_depth",
    "contention_rate",
    "mutex_utilization",
    "deadlock",
    "score"
]

with open("deadlock_dataset.csv", "w", newline="") as f:

    writer = csv.writer(f)

    writer.writerow(header)

    for sample in range(NUM_SAMPLES):

        if sample % 1000 == 0:
            print("Generated:", sample)

        sim = RTOSSimulator(
            random.randint(2,20),
            random.randint(2,10)
        )

        for _ in range(
            random.randint(50,200)
        ):
            sim.random_event()

        if random.random() < 0.20:
            sim.inject_deadlock()

        writer.writerow(
            extract_features(sim)
        )

print("Dataset saved.")