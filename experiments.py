#!/usr/bin/python3

import csv
import os
import re
import subprocess

import tqdm

from math import sqrt, log2

EXPERIMENT_FOLDER = "experiment_results"

# list of tuples (dataset, #attributes)
DATASETS = [
        ("audiology.standardized", 69),
        ("crx", 15),
        ("hepatitis", 19),
        ("house-votes-84", 16),
        ("iris", 4),
        ("kr-vs-kp", 36),
        ("lenses", 4),
        ("soybean-small", 35),
        ("splice", 60),
        ("zoo", 17),
]


RANK_TEMPLATE = r"""\begin{enumerate}
{}
\end{enumerate}"""

ACCURACY_MATCHER = re.compile(
        r"Accuracy: (?P<avg>[0-9]+(?:\.[0-9]+)?)+-(?P<std>[0-9]+(?:\.[0-9]+)?)%")
ELAPSED_MATCHER = re.compile(
        r"Elapsed: (?P<avg>[0-9]+(?:\.[0-9]+)?)+-(?P<std>[0-9]+(?:\.[0-9]+)?)s")

def feature_rank(dataset, ntrees, f, minsplit=10):
    cmd = ["./build/train_and_test", "-M{}".format(ntrees), "-F{}".format(f),
           "-N{}".format(minsplit), dataset]
    out = subprocess.check_output(cmd).decode("ascii")
    frank = out.split("\n")[1:-1]
    frank = "\n".join(map(lambda s: r"\item "+s, frank))
    frank = "\\begin{enumerate}\n" + frank + "\n\\end{enumerate}"
    return frank.replace("_","\\_")

def accuracy_and_time(dataset, ntrees, f, minsplit=10):
    cmd = ["./build/train_and_test", "-M{}".format(ntrees), "-F{}".format(f),
           "-N{}".format(minsplit), "--cv", "5", dataset]
    out = subprocess.check_output(cmd).decode("ascii")
    out = out.split("\n")[-3:-1]
    out[0] = out[0][10:-1]
    out[1] = out[1][9:-1]
    avg_acc, std_acc = out[0].split("+-")
    avg_el, std_el = out[1].split("+-")
    acc_str = "{:.02f} $ \\pm $ {:.02f}".format(float(avg_acc), float(std_acc))
    el_str = "{:.02f} $ \\pm $ {:.02f}".format(float(avg_el), float(std_el))
    return acc_str, el_str

if __name__ == "__main__":
    for dataset, nattr in DATASETS:
        dataset_folder = os.path.join(EXPERIMENT_FOLDER, dataset)
        try:
            os.makedirs(dataset_folder)
        except OSError:
            pass # already exists
        fcand = [1, 3, int(log2(nattr)+1), int(round(sqrt(nattr)))]
        table_acc = []
        table_elapsed = []
        for ntrees in [50, 100]:
            row_acc = []
            row_elapsed = []
            for f in fcand:
                frank = feature_rank(dataset, ntrees, f)
                rankfile = os.path.join(dataset_folder,
                        "rank_{}_{}.tex".format(ntrees, f))
                acc_str, el_str = accuracy_and_time(dataset, ntrees, f)
                row_acc.append(acc_str)
                row_elapsed.append(el_str)
                with open(rankfile, "w") as f:
                    f.write(frank)
            table_acc.append(row_acc)
            table_elapsed.append(row_elapsed)
        table_acc_file = os.path.join(dataset_folder, "accuracy.csv")
        table_elapsed_file = os.path.join(dataset_folder, "elapsed.csv")
        with open(table_acc_file, "w") as f:
            writer = csv.writer(f)
            writer.writerows(table_acc)
        with open(table_elapsed_file, "w") as f:
            writer = csv.writer(f)
            writer.writerows(table_elapsed)

    # print(accuracy_and_time("iris", 10, 2))

