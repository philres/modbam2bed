#include <stdlib.h>
#include <string.h>
#include <argp.h>

#include "args.h"

const char *argp_program_version = "0.0.4";
const char *argp_program_bug_address = "chris.wright@nanoporetech.com";
static char doc[] = 
 "modbam2bed -- summarise a BAM with modified base tags to bedMethyl.\
 \vPositions absent from the methylation tags are assumed to be canonical. \
 Positions with modified probability between upper and lower thresholds\
 are removed from the counting process. Column 5 (\"score\") of the output\
 is calculated as the proportion of bases called as the canonical or modified\
 reference base with respect to the number of spanning reads, scaled to a\
 maximum of 1000. Column 11 is the percentage of reference base calls identified\
 as being modified.";
static char args_doc[] = " <reads.bam> <reference.fasta> > ";
static struct argp_option options[] = {
    {"canon_threshold", 'a', "CANON_THRESHOLD", 0,
        "Bases with mod. probability < CANON_THRESHOLD are counted as canonical."},
    {"mod_threshold", 'b', "MOD_THRESHOLD", 0,
        "Bases with mod. probability > MOD_THRESHOLD are counted as modified."},
    {"mod_base", 'm', "MODIFIED_BASE", 0,
        "Modified base of interest, one of: 5mC, 5hmC, 5fC, 5caC, 5hmU, 5fU, 5caU, 6mA, 5oxoG, Xao."},
    {"cpg", 'c', 0, 0,
        "Output records filtered to CpG sited."},
    {"region", 'r', "chr:start-end", 0,
        "Genomic region to process."},
    {"read_group", 'g', "READ_GROUP", 0,
        "Only process reads from given read group."},
    {"extended", 'e', 0, 0,
        "Output extended bedMethyl including counts of canonical, modified, and filtered bases (in that order)."},
    {"threads", 't', "THREADS", 0,
        "Number of threads for BAM processing."},
    { 0 }
};


static error_t parse_opt (int key, char *arg, struct argp_state *state) {
    arguments_t *arguments = state->input;
    float thresh;
    bool found = false;
    switch (key) {
        case 'a':
            thresh = atof(arg);
            if (thresh < 0 || thresh > 1.0) {
                argp_error (state, "Threshold parameter must be in (0,1), got %s", arg);
            }
            arguments->lowthreshold = (int)(thresh * 255);
            break;
        case 'b':
            thresh = atof(arg);
            if (thresh < 0 || thresh > 1.0) {
                argp_error (state, "Threshold parameter must be in (0,1), got %s", arg);
            }
            arguments->highthreshold = (int)(thresh * 255);
            break;
        case 'm':
            for (size_t i = 0; i < n_mod_bases; ++i) {
                if (!strcmp(mod_bases[i].abbrev, arg)) {
                    arguments->mod_base = mod_bases[i];
                    found = true;
                    break;
                }
            }
            if (!found) {
                argp_error (state, "Unrecognised modified base type: '%s'.", arg);
            }
            break;
        case 'r':
            arguments->region = arg;
            break;
        case 'c':
            arguments->cpg = true;
            break;
        case 'e':
            arguments->extended = true;
            break;
        case 'g':
            arguments->read_group = arg;
            break;
        case 't':
            arguments->threads = atoi(arg);
            break;
        case ARGP_KEY_NO_ARGS:
            argp_usage (state);
            break;
        case ARGP_KEY_ARG:
            switch (state->arg_num) {
                case 0:
                    arguments->bam = arg;
                    break;
                case 1:
                    arguments->ref = arg;
                    break;
                default:
                    argp_usage (state);
            }
            break;
        case ARGP_KEY_END:
            if (state->arg_num < 2)
                argp_usage (state);
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = {options, parse_opt, args_doc, doc};

arguments_t parse_arguments(int argc, char** argv) {
    arguments_t args;
    args.mod_base = default_mod_base;
    args.lowthreshold = (int)(0.33 * 255);
    args.highthreshold = (int)(0.66 * 255);
    args.bam = NULL;
    args.ref = NULL;
    args.region = NULL;
    args.read_group = NULL;
    args.cpg = false;
    args.extended = false;
    args.threads = 1;
    argp_parse(&argp, argc, argv, 0, 0, &args);
    // allow CpG only for C!
    if(args.cpg) {
        if (args.mod_base.base != 'C') {
            fprintf(stderr, "Option '--cpg' can only be used with cytosine modifications.");
            exit(1);
        }; 
    }
    return args;
}
