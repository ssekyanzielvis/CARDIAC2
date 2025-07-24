# Define the word and parameters
word = "💓CAROLINE"
max_lines = 20  # Number of lines for the triangle
max_repeats = 50  # Maximum repetitions on the bottom line (scaled down from 1000 for practicality)

# Calculate the step for increasing repetitions
repetitions = [int(1 + i * (max_repeats - 1) / (max_lines - 1)) for i in range(max_lines)]

# Find the width of the longest line for centering
max_width = len(word) * max_repeats + (max_repeats - 1) * 1  # 1 space between words

# Print the triangle
for i in range(max_lines):
    # Create the line with the current number of repetitions
    line = " ".join([word] * repetitions[i])
    # Center the line
    centered_line = line.center(max_width)
    print(centered_line)