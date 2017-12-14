# Custom implementation of Random Forest
*by Alejandro Suárez*

This is my implementation of Random Forest for classification in C++11.
It accepts several parameters and is able to learn and classify data sets in CSV format.
Our Random Forests can deal with both numerical and categorical attributes.
Moreover, trees can be stored in a JSON file and/or visualized as .dot
graphs. The implementation does very basic handling of missing values:
in the training set, it substitutes them by the class median (for
numeric attributes) and the per-class mode (for categorical attributes). In
the test set, the missing values are substittued by the global median/mode.

This implementation has been coded mainly for experimentation purposes and it
does not aim at outperforming any other algorithm (although it performs
quite well).

## How to build

This project has been tested in Ubuntu 16.04 with g++5.4.0 and make 4.1. It has
mainly two external single header dependencies:

- Richberger's [args](https://github.com/Taywee/args) for parsing
command line arguments (all the forest parameters are specified like this).
- Lohmann's [json](https://github.com/nlohmann/json) for serializing and
deserializing forests in JSON format so they can be loaded afterwards.

The headers of these dependencies are already shipped with my code, so no
action is required by the user to install them.

In order to build the project, simply run `make` from this (`README.md`'s
location) folder. This will create a folder called build and compile all
the object and binaries inside. The most important binary is
`train_and_test` (the other ones are modular tests).

## Usage

The program's help (shown invoking `./build/train_and_test --help`) reads as follows:

```
  ./build/train_and_test [datasetname] {OPTIONS}

    Train and/or test a random forest with an arbitrary data set

  OPTIONS:

      -h, --help                        Display this help menu
      -v[verbose_level],
      --verbose=[verbose_level]         0: no info, 1: elapsed train time,
                                        accuracy and feature weights, if
                                        applicable (default); 2: stats; 3+:
                                        options
      -v[seed], --verbose=[seed]        RNG seed
      -l[filename], --load=[filename]   Load forest from JSON, instead of
                                        training from scratch
      Train parameters
        -M[ntrees], --ntrees=[ntrees]     Number of trees in the ensemble
                                          (default 10)
        -F[f], --feature-bag=[f]          Number of features evaluated randomly
                                          at each split (sqrt of the number of
                                          attributes if not specified)
        -N[n], --min-split=[n]            Minimum number of instances to split
                                          (default 2)
        Metric
          -g, --gini                        Gini impurity (default)
          -i, --entropy                     Entropy (information gain)
          -e, --error                       Error (1 - pmax)
        --cv=[cv]                         Cross validation (by default, no cross
                                          validation is performed)
        -j[filename], --json=[filename]   Store forest in JSON format
        -d[prefix], --dot=[prefix]        Create dot files
      datasetname                       Name of the data set (default iris).
      "--" can be used to terminate flag options and force all following
      arguments to be treated as positional options
```

There are mainly three use cases:

### Train and save

In this use case, the program is invoked with the training parameters and (optionally) with
the `-j` and `-d` options. It will learn an ensemble of trees using one
of the data sets inside the `Data` folder (more can be added following the
same convention as the ones already included). It will not perform any
training/test split. The learned model will be stored in a JSON file (if
the `-j` option is given) and as several DOT files ready for being
processed and visualized. At the end of the training process, the ranking of features
and their score (based on how often they have been chosen as splitting criterion in
a stump) is displayed. In order to use the program like this, the
`--cv` and `-l` flags must be ommited.

Example usage:
```
$ ./build/train_and_test -M3 -F4 -jhepatitis_3_4.json -dhepatitis -v3 hepatitis
Verbose: 3
Train: true
ntrees: 3
f (<= 0 means sqrt of #attributes): 4
n: 2
Metric: gini
cv: 0
save to json: hepatitis_3_4.json
dot prefix: hepatitis
rng seed: 42
data set: hepatitis
----------------------------------
#Columns: 19 (+ 1 target)
#Records: 155
Missing values: 5.67063%
Target column: Class
View? no
Attributes:
  Class (categoric): pdf: {1:20.6452%,2:79.3548%}
  AGE (numeric): min: 7, max: 78, mean: 41.2, stdev: 12.5253
  SEX (categoric): pdf: {1:89.6774%,2:10.3226%}
  STEROID (categoric): pdf: {1:49.0323%,2:50.3226%,?:0.645161%}
  ANTIVIRALS (categoric): pdf: {1:15.4839%,2:84.5161%}
  FATIGUE (categoric): pdf: {1:64.5161%,2:34.8387%,?:0.645161%}
  MALAISE (categoric): pdf: {1:39.3548%,2:60%,?:0.645161%}
  ANOREXIA (categoric): pdf: {1:20.6452%,2:78.7097%,?:0.645161%}
  LIVER-BIG (categoric): pdf: {1:16.129%,2:77.4194%,?:6.45161%}
  LIVER-FIRM (categoric): pdf: {1:38.7097%,2:54.1935%,?:7.09677%}
  SPLEEN-PALPABLE (categoric): pdf: {1:19.3548%,2:77.4194%,?:3.22581%}
  SPIDERS (categoric): pdf: {1:32.9032%,2:63.871%,?:3.22581%}
  ASCITES (categoric): pdf: {1:12.9032%,2:83.871%,?:3.22581%}
  VARICES (categoric): pdf: {1:11.6129%,2:85.1613%,?:3.22581%}
  BILIRUBIN (numeric): min: 0.3, max: 8, mean: 1.37226, stdev: 1.21605
  ALK-PHOSPHATE (numeric): min: 26, max: 295, mean: 85.6194, stdev: 61.8612
  SGOT (numeric): min: 14, max: 648, mean: 83.6774, stdev: 89.2384
  ALBUMIN (numeric): min: 2.1, max: 6.4, mean: 3.42323, stdev: 1.31408
  PROTIME (numeric): min: 0, max: 100, mean: 35.1161, stdev: 35.1081
  HISTOLOGY (categoric): pdf: {1:54.8387%,2:45.1613%}
1: [2,30.000000,2,1,2,2,2,2,1,2,2,2,2,2,1.000000,85.000000,18.000000,4.000000,?,1]
2: [2,50.000000,1,1,2,1,2,2,1,2,2,2,2,2,0.900000,135.000000,42.000000,3.500000,?,1]
...
155: [1,43.000000,1,2,2,1,2,2,2,2,1,1,1,2,1.200000,100.000000,19.000000,3.100000,42.000000,2]
----------------------------------

Data set has missing values. Using per-class median/mode imputation...
Feature ranking:
ALK-PHOSPHATE (0.164384)
PROTIME (0.136986)
ALBUMIN (0.109589)
BILIRUBIN (0.0958904)
SGOT (0.0821918)
SPIDERS (0.0684932)
AGE (0.0684932)
VARICES (0.0410959)
LIVER-BIG (0.0410959)
HISTOLOGY (0.0410959)
STEROID (0.0273973)
MALAISE (0.0273973)
ASCITES (0.0273973)
ANOREXIA (0.0273973)
SEX (0.0136986)
LIVER-FIRM (0.0136986)
FATIGUE (0.0136986)
$ ls
build               Data                dot                 forests
hepatitis1.dot      hepatitis2.dot      hepatitis_3_4.json  hepatitis3.dot
LICENSE             Makefile            README.md           src
```

In this example we train a forest with 3 trees (`-M3`). At each splitting point
the learner chooses the best split candidate from 4 randomly chosen features
(`-F4`). The resulting forest is stored in the `hepatitis_3_4.json` file and
a dot file (with the hepatitis prefix) is created for each of the trees in the
forest, ready for being processed with Graphviz. Example of how to render a dot
file:

```
dot -Tpdf hepatitis1.dot -o hepatitis1.pdf
```

The result is a PDF with rendered graph similar to the following one (this one has been actually rendered with `-Tsvg` instead of `-Tpdf`):
![Hepatitis decision tree](img/hepatitis/hepatitis_example.svg)


### Train and evaluate

The program evaluates a certain set of Random Forest parameters via Cross-Validation.
That is, the data is splitted into several folds. It trains as many models as folds,
each time using one of the folds as test set and the remaining ones as training data.
The accuracy and time results are averaged and printed at the end of
the evaluation. To use the program like this, the `-l` flag must be ommited, and `--cv`
must be set to an integer value greater than 1. The `-j` and `-d` flags are ignored.

Let us see how it is done in a practical example:
```
$ ./build/train_and_test -M20 --min-split=6 --cv=5 hepatitis
Data set has missing value. Using per-class median/mode imputation in
train set and global median/mode imputation in test set.
Fold 1: accuracy = 80.6452%; elapsed(CPU) = 0.022932s
Fold 2: accuracy = 87.0968%; elapsed(CPU) = 0.010616s
Fold 3: accuracy = 77.4194%; elapsed(CPU) = 0.008732s
Fold 4: accuracy = 83.871%; elapsed(CPU) = 0.00788s
Fold 5: accuracy = 87.0968%; elapsed(CPU) = 0.00792s
Accuracy: 83.2258+-3.7619%
Elapsed: 0.011616+-0.00574434s
```

This trains and evaluates several random forests of size 20 via 5-fold cross
validation.

### Load and evaluate

Loads a previously stored forest (see first use case) and evaluates it against a
different data set. All the training parameters are ignored. It is enough to
indicate the JSON file (`-l` option) and the target data set.

Next we show an example of this use case:

```
$ ./build/train_and_test -lhepatitis_3_4.json hepatitis
Accuracy: 94.1935%
```

This is the same model we trained in the first use case. Notice that we are evaluating
the same data the model has been trained with, so it is only natural that the
accuracy is that high. It goes without saying that the reported accuracy is meaningful
only when the learner has not seen the test data (this example is just for illustration
purposes).

## To-do

The source code has not been fully documented.


