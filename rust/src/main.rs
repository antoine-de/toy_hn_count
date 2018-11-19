extern crate csv;
extern crate itertools;
extern crate structopt;

use itertools::Itertools;
use std::collections::HashMap;
use std::path::PathBuf;
use structopt::StructOpt;

#[derive(StructOpt, Debug)]
struct CommonParams {
    #[structopt(long = "from")]
    from: Option<u64>,
    #[structopt(long = "to")]
    to: Option<u64>,
    #[structopt(name = "INPUT_FILE", parse(from_os_str))]
    file: PathBuf,
}

#[derive(StructOpt, Debug)]
#[structopt(name = "hn_stat")]
enum Params {
    #[structopt(name = "distinct")]
    Distinct {
        #[structopt(flatten)]
        common_params: CommonParams,
    },
    #[structopt(name = "top")]
    Top {
        #[structopt(name = "nb_top_queries")]
        nb_queries: usize,
        #[structopt(flatten)]
        common_params: CommonParams,
    },
}

fn count_entries(file: PathBuf, from: Option<u64>, to: Option<u64>) -> HashMap<String, u64> {
    let mut entries = HashMap::new();
    for (_ts, url) in csv::ReaderBuilder::new()
        .delimiter(b'\t')
        .has_headers(false)
        .from_path(file)
        .expect("impossible to open file")
        .records()
        .filter_map(Result::ok)
        .filter_map(|s| {
            s.get(0)
                .and_then(|ts| ts.parse::<u64>().ok())
                .and_then(|ts| s.get(1).map(|url| (ts, url.to_string())))
        }).filter(|(ts, _)| from.as_ref().map(|f| ts >= f).unwrap_or(true))
        .filter(|(ts, _)| to.as_ref().map(|t| ts <= t).unwrap_or(true))
    {
        *entries.entry(url).or_insert(0) += 1;
    }
    entries
}

fn compute_distinct(common_params: CommonParams) {
    let entries = count_entries(common_params.file, common_params.from, common_params.to);

    println!("{}", entries.len());
}

fn compute_top(nb_queries: usize, common_params: CommonParams) {
    let entries = count_entries(common_params.file, common_params.from, common_params.to);

    // only the global filtering is implemented for the rust version
    entries
        .into_iter()
        .collect::<Vec<_>>()
        .into_iter()
        .sorted_by(|a, b| Ord::cmp(&b.1, &a.1))
        .into_iter()
        .take(nb_queries)
        .for_each(|(url, count)| println!("{} {}", url, count));
}

fn main() {
    let params = Params::from_args();

    match params {
        Params::Distinct { common_params } => compute_distinct(common_params),
        Params::Top {
            nb_queries,
            common_params,
        } => compute_top(nb_queries, common_params),
    }
}
