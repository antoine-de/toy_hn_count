# Project

This is a toy project done for an interview to process a TSV [data file](https://www.dropbox.com/s/4qseadi3lcceq3b/hn_logs.tsv.bz2?dl=0) listing all queries performed on [HN Search](https://hn.algolia.com) during a few days.

## Building

In the directory:
`make`

## Run

`./hnStat distinct [--from TIMESTAMP] [--to TIMESTAMP] <input_file>`

will output the number of distinct entries in the file

```
./hnStat top <nb_top_queries> [--from TIMESTAMP] [--to TIMESTAMP] <input_file>
```

will output the `nb_top_queries` most popular entries.

## Results

To compare the results, we can use simple `bash` tools:

The distinct command can be emulated wih:
```
 % cut -d$'\t' -f2  hn_logs.tsv | sort | uniq | wc -l
573698
```

Note: The results are not exactly the same, since the first line of the line is not correct, and is skipped by the c++ project

The top command can be emulated with:
```
cut -d$'\t' -f2  hn_logs.tsv | sort | uniq -c | sort -n -r | head
```
## Performances

On my machine, here are some figures (each test has been run 3 times, and it's the minimum value that is displayed):


```
time ./hnStat distinct hn_logs.tsv > /dev/null

real    0m1.670s
user    0m1.549s
sys     0m0.121s
```

```
time ./hnStat top 10 hn_logs.tsv > /dev/null

real    0m1.796s
user    0m1.708s
sys     0m0.088s
```

```
time ./hnStat top 100 hn_logs.tsv > /dev/null

real    0m1.809s
user    0m1.717s
sys     0m0.092s
```

```
time ./hnStat top 10000000 hn_logs.tsv > /dev/null

real    0m2.331s
user    0m2.025s
sys     0m0.296s
```

## Various comments

* The test was not to use any external library, so a lot of this code could be greatly improved by using boost or range-v3 (and there are not unit tests)
* The algorithms are explained as comment above the methods `compute_distinct` and `compute_top`

## Rust version

For fun there is also a Rust version available in the `rust/` directory. It has better performance, but I think it's mainly due to a better file parsing.
