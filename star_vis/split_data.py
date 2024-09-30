import pandas as pd
import os

# Function to split DataFrame into chunks and save to separate CSV files
def split_and_save_csv(input_file, output_dir, chunk_size=50):
    # Read the CSV file into a DataFrame
    df = pd.read_csv(input_file)

    # Create output directory if it doesn't exist
    os.makedirs(output_dir, exist_ok=True)

    # Iterate through the DataFrame in chunks
    for i in range(0, len(df), chunk_size):
        chunk = df.iloc[i:i + chunk_size]
        chunk = chunk.iloc[:, 1:] 
        output_file = os.path.join(output_dir, f'sample_{i // chunk_size + 1}.csv')
        chunk.to_csv(output_file, index=False, header=False)
        print(f'Saved: {output_file}')

# Example usage
input_csv_file = 'data/filtered_starcat.csv'  # Replace with your input file path
output_directory = 'data/samples'       # Replace with your desired output directory
split_and_save_csv(input_csv_file, output_directory)
