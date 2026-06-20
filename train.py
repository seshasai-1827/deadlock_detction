import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.ensemble import RandomForestRegressor
from sklearn.metrics import mean_absolute_error, r2_score
import emlearn

df = pd.read_csv("./simulator/deadlock_dataset.csv")

X = df.drop(
    ["score", "deadlock"],
    axis=1
)

y = df["score"]

X_train, X_test, y_train, y_test = train_test_split(
    X,
    y,
    test_size=0.2,
    random_state=42
)

rf = RandomForestRegressor(
    n_estimators=20,
    max_depth=6,
    min_samples_leaf=5,
    n_jobs=-1,
    random_state=42
)

rf.fit(X_train, y_train)

pred = rf.predict(X_test)

print("MAE =", mean_absolute_error(y_test, pred))
print("R²  =", r2_score(y_test, pred))

importance = pd.DataFrame({
    "Feature": X.columns,
    "Importance": rf.feature_importances_
})

print(
    importance.sort_values(
        "Importance",
        ascending=False
    )
)

cmodel = emlearn.convert(rf)
cmodel.save(
    file = "deadlock_rf.h",
    name="deadlock_rf"
)