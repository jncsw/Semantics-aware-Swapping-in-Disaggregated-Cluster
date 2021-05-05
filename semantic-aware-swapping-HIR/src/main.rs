#![feature(rustc_private)]
extern crate rustc_error_codes;
extern crate rustc_errors;
extern crate rustc_hash;
extern crate rustc_hir;
extern crate rustc_interface;
extern crate rustc_session;
extern crate rustc_span;
extern crate rustc_ast_pretty;
use rustc_ast_pretty::pprust::item_to_string;
use rustc_errors::registry;
use rustc_hash::{FxHashMap, FxHashSet};
use rustc_session::config;
use rustc_span::source_map;
use std::path;
use std::process;
use std::str;
use std::fs;
use clap::Clap;
use std::collections::HashMap;
#[derive(Clap)]
#[clap(version = "0.1")]
struct Args {
    src: String,
}
fn main() {
    let args = Args::parse();
    let src_code = fs::read_to_string(args.src).expect("Read source code.");
    let out = process::Command::new("rustc")
        .arg("--print=sysroot")
        .current_dir(".")
        .output()
        .unwrap();
    let sysroot = str::from_utf8(&out.stdout).unwrap().trim();
    let config = rustc_interface::Config {
        // Command line options
        opts: config::Options {
            maybe_sysroot: Some(path::PathBuf::from(sysroot)),
            ..config::Options::default()
        },
        // cfg! configuration in addition to the default ones
        crate_cfg: FxHashSet::default(), // FxHashSet<(String, Option<String>)>
        input: config::Input::Str {
            name: source_map::FileName::Custom("main.rs".to_string()),
            input: src_code,
        },
        input_path: None,  // Option<PathBuf>
        output_dir: None,  // Option<PathBuf>
        output_file: None, // Option<PathBuf>
        file_loader: None, // Option<Box<dyn FileLoader + Send + Sync>>
        diagnostic_output: rustc_session::DiagnosticOutput::Default,
        // Set to capture stderr output during compiler execution
        stderr: None,                    // Option<Arc<Mutex<Vec<u8>>>>
        // crate_name: None,                // Option<String>
        lint_caps: FxHashMap::default(), // FxHashMap<lint::LintId, lint::Level>
        // This is a callback from the driver that is called when we're registering lints;
        // it is called during plugin registration when we have the LintStore in a non-shared state.
        //
        // Note that if you find a Some here you probably want to call that function in the new
        // function being registered.
        register_lints: None, // Option<Box<dyn Fn(&Session, &mut LintStore) + Send + Sync>>
        // This is a callback from the driver that is called just after we have populated
        // the list of queries.
        //
        // The second parameter is local providers and the third parameter is external providers.
        override_queries: None, // Option<fn(&Session, &mut ty::query::Providers<'_>, &mut ty::query::Providers<'_>)>
        // Registry of diagnostics codes.
        registry: registry::Registry::new(&rustc_error_codes::DIAGNOSTICS),
        make_codegen_backend: None,
        parse_sess_created: None,
    };
    let mut freqs: HashMap<rustc_hir::HirId, i32> = HashMap::new();
    rustc_interface::run_compiler(config, |compiler| {
        compiler.enter(|queries| {
            // TODO: add this to -Z unpretty
            let ast_krate = queries.parse().unwrap().take();
            for item in ast_krate.items {
                println!("{}", item_to_string(&item));
            }
            println!("Analysis:");
            // Analyze the crate and inspect the types under the cursor.
            queries.global_ctxt().unwrap().take().enter(|tcx| {
                // Every compilation contains a single crate.
                let hir_krate = tcx.hir().krate();
                // Iterate over the top-level items in the crate, looking for the main function.
                for (_, item) in &hir_krate.items {
                    // Use pattern-matching to find a specific node inside the main function.
                    if let rustc_hir::ItemKind::Fn(_, _, body_id) = item.kind {
                        let expr = &tcx.hir().body(body_id).value;
                        if let rustc_hir::ExprKind::Block(block, _) = expr.kind {
                            // println!("{:?}", block);
                            for stmt in block.stmts {
                                if let rustc_hir::StmtKind::Semi(expr) = stmt.kind {
                                    if let rustc_hir::ExprKind::Assign(left, right, _) = expr.kind {
                                        match &left.kind {
                                            rustc_hir::ExprKind::Path(id_path) => {
                                                // TODO: may need to consider other cases of lhs.
                                                if let rustc_hir::QPath::Resolved(_, id) = id_path {
                                                    let rustc_hir::Path{span:_, res, segments: _} = id;
                                                    if let rustc_hir::def::Res::Local(local_id) = res {
                                                        *freqs.entry(local_id.clone()).or_default() += 1;
                                                        if let rustc_hir::Node::Binding(pat) = tcx.hir().find(local_id.clone()).unwrap() {
                                                            // Probably should use each_binding() to access each binding,
                                                            // to handle cases like destructuring, or match statement
                                                            pat.simple_ident().map(|id_sym| {
                                                                println!("Assign to Variable: {:#?} with rhs: \n {:#?}", id_sym, right);
                                                            });
                                                        }
                                                    }
                                                }
                                            }
                                            rustc_hir::ExprKind::Index(arr, _) => {
                                                if let rustc_hir::ExprKind::Path(rustc_hir::QPath::Resolved(_, id)) = arr.kind {
                                                    let rustc_hir::Path{span:_, res, segments: _} = id;
                                                    if let rustc_hir::def::Res::Local(local_id) = res {
                                                        *freqs.entry(local_id.clone()).or_default() += 1;
                                                        if let rustc_hir::Node::Binding(pat) = tcx.hir().find(local_id.clone()).unwrap() {
                                                            pat.simple_ident().map(|id_sym| {
                                                                println!("Assign to Variable: {:#?} with rhs: \n {:#?}", id_sym, right);
                                                            });
                                                        }
                                                    }
                                                }
                                            }
                                            _ => println!("TODO: Extend handling of other expression kinds here. ")
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                for (key, f) in freqs {
                    if let rustc_hir::Node::Binding(pat) = tcx.hir().find(key.clone()).unwrap() {
                        pat.simple_ident().map(|id_sym| {
                                println!("Assign to Variable: {:#?}, frequency: \n {:#?}", id_sym, f);
                        });
                    }
                }
            })
        });
    });
}