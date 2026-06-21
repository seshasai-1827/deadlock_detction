import pandas as pd

from sklearn.model_selection import train_test_split
from sklearn.ensemble import RandomForestRegressor
from sklearn.metrics import mean_absolute_error
from sklearn.metrics import r2_score

import emlearn

# ==========================================
# Load Dataset
# ==========================================

df = pd.read_csv(
    "./simulator/dining_philosophers_dataset.csv"
)

# ==========================================
# Features used on STM32
# ==========================================

X = df[
[
    "blocked_tasks",
    "blocked_ratio",
    "edge_count",
    "graph_density",
    "mutex_utilization"
]
]

y = df["score"]

# ==========================================
# Train/Test Split
# ==========================================

X_train, X_test, y_train, y_test = train_test_split(
    X,
    y,
    test_size=0.2,
    random_state=42
)

# ==========================================
# Random Forest
# ==========================================

rf = RandomForestRegressor(
    n_estimators=20,
    max_depth=6,
    min_samples_leaf=5,
    random_state=42,
    n_jobs=-1
)

rf.fit(
    X_train,
    y_train
)

# ==========================================
# Evaluation
# ==========================================

pred = rf.predict(
    X_test
)

print(
    "MAE =",
    mean_absolute_error(
        y_test,
        pred
    )
)

print(
    "R² =",
    r2_score(
        y_test,
        pred
    )
)

# ==========================================
# Feature Importance
# ==========================================

importance = pd.DataFrame(
{
    "Feature": X.columns,
    "Importance": rf.feature_importances_
}
)

print(
    importance.sort_values(
        "Importance",
        ascending=False
    )
)

# ==========================================
# Export to STM32
# ==========================================

cmodel = emlearn.convert(rf)

cmodel.save(
    file="deadlock_rf.h",
    name="deadlock_rf"
)

print(
    "\nModel exported to deadlock_rf.h"
)