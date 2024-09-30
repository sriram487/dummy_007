#pylint: skip-file
import pandas as pd

file_path = './starcat.tsv'
data = pd.read_csv(file_path, sep='\t', skiprows=54)
data.columns = ['_RAJ2000', '_DEJ2000', 'HIP', 'RArad', 'DErad', 'Plx', 'pmRA', 'pmDE', 'Hpmag', 'B-V']

# filtering data based up on Magnatitude value
filtered_data = data[(data['Hpmag'] >= 0) & (data['Hpmag'] <= 6)]
# print(filtered_data.head())
# print(len(filtered_data))

# Select the desired columns
columns_to_save = ['HIP', 'RArad', 'DErad', 'Hpmag']
filtered_subset = filtered_data[columns_to_save]

# Write to a CSV file
filtered_subset.to_csv('filtered_starcat.csv', index=False)
