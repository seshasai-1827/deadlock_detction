import pandas as pd
df = pd.read_csv("./simulator/dining_philosophers_dataset.csv")

print(df.head())
print(df.describe())

print(df["deadlock"].value_counts())