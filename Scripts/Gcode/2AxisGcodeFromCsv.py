import datetime
import math

import matplotlib.pyplot as plt
import csv
import argparse

from gcode_writer import generate_2_axis_gcode

SPEED = 5  # mm/s


def generate_gcode_from_csv(input_filename, output_filename, show_plot=True):
    x = []
    y = []
    print(f"Opening {input_filename} ...")
    with open(input_filename, newline='') as csvfile:
        reader = csv.reader(csvfile)
        for row in reader:
            x.append(float(row[0]))
            y.append(float(row[1]))

    generate_2_axis_gcode(x, y, input_filename, output_filename)


    if show_plot:
        print("Showing plot...")
        plot(x, y, input_filename.replace('\\', '/').split('/')[-1:][0])
        print("Closing...")


def plot(x_series, y_series, plot_name):
    max_x, max_y = 0, 0
    for x in x_series:
        max_x = abs(x) if abs(x) > max_x else max_x
    for y in y_series:
        max_y = abs(y) if abs(y) > max_y else max_y


    plt.style.use('dark_background')
    fig, ax = plt.subplots()
    ax.plot(x_series, y_series, "#ff0000")
    plt.plot(x_series[0], y_series[0], marker="o", markersize=10, markeredgecolor="#00ff00", markerfacecolor="#00000000")

    ax.set(xlabel='mm', ylabel='mm', title=plot_name)

    ax.grid()

    if max_x > max_y:
        plt.xlim(-max_x * 0.1, max_x + max_x * 0.1)
        plt.ylim((-max_x / 2) - max_x * 0.1, (max_x / 2) + max_x * 0.1)
    else:
        plt.xlim(-max_y * 0.1, max_y * 2 + max_y * 0.1)
        plt.ylim(-max_y * 1.1, max_y * 1.1)

    plt.show()


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("csv_file", help="The csv file you'd like to convert to gcode")
    parser.add_argument("-o", "--output", help="The output gcode path/filename", required=False, default="out.gcode")
    args = parser.parse_args()
    generate_gcode_from_csv(args.csv_file, args.output)
