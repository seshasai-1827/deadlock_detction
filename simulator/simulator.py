import csv
import random

NUM_SAMPLES = 50000

# ==========================================
# WFG Construction
# ==========================================

def build_graph(waiting_for, owns_fork):

    graph = [[0]*5 for _ in range(5)]

    for i in range(5):

        if waiting_for[i] != -1:

            fork = waiting_for[i]

            for j in range(5):

                if owns_fork[j] == fork:

                    graph[i][j] = 1

    return graph

# ==========================================
# DFS Deadlock Detection
# ==========================================

def detect_deadlock(graph):

    visited = [0]*5
    stack = [0]*5

    def dfs(node):

        visited[node] = 1
        stack[node] = 1

        for v in range(5):

            if graph[node][v]:

                if not visited[v]:

                    if dfs(v):
                        return True

                elif stack[v]:
                    return True

        stack[node] = 0
        return False

    for i in range(5):

        if not visited[i]:

            if dfs(i):
                return True

    return False

# ==========================================
# Dataset Generation
# ==========================================

header = [
    "blocked_tasks",
    "blocked_ratio",
    "edge_count",
    "graph_density",
    "mutex_utilization",
    "deadlock",
    "score"
]

with open("dining_philosophers_dataset.csv",
          "w",
          newline="") as f:

    writer = csv.writer(f)

    writer.writerow(header)

    for sample in range(NUM_SAMPLES):

        waiting_for = [-1]*5
        owns_fork = [-1]*5

        # -------------------------
        # normal operating states
        # -------------------------

        mode = random.random()

        if mode < 0.25:

            # deadlock state

            for i in range(5):

                owns_fork[i] = i
                waiting_for[i] = (i+1)%5

        else:

            for i in range(5):

                if random.random() < 0.5:

                    owns_fork[i] = i

                if random.random() < 0.4:

                    waiting_for[i] = random.randint(0,4)

        graph = build_graph(
            waiting_for,
            owns_fork
        )

        deadlock = detect_deadlock(
            graph
        )

        blocked_tasks = sum(
            1 for x in waiting_for
            if x != -1
        )

        edge_count = 0

        for i in range(5):
            for j in range(5):

                if graph[i][j]:
                    edge_count += 1

        blocked_ratio = (
            blocked_tasks / 5.0
        )

        graph_density = (
            edge_count / 20.0
        )

        mutex_utilization = (
            sum(
                1 for x in owns_fork
                if x != -1
            ) / 5.0
        )

        # -------------------------
        # Risk Score
        # -------------------------

        score = (
            0.45 * blocked_ratio +
            0.25 * graph_density +
            0.20 * mutex_utilization
        )

        if deadlock:
            score += 0.30

        score = min(score, 1.0)

        writer.writerow([
            blocked_tasks,
            blocked_ratio,
            edge_count,
            graph_density,
            mutex_utilization,
            int(deadlock),
            score
        ])

print("Dataset generated.")