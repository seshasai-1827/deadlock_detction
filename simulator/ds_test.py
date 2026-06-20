import pandas as pd
df = pd.read_csv("deadlock_dataset.csv")

print(df.head())
print(df.describe())

print(df["deadlock"].value_counts())