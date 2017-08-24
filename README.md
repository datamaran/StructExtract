# Datamaran 

Datamaran is a toolkit for extracting structure from semi-structured log datasets without human supervision.

## Build

```
cd StructExtract
make datamaran
mv datamaran ../
```

## Usage

```
./datamaran sample.txt output
```

This usually takes a few minutes. When it finishes, it will generate a series of output files with name: output_?.tsv respectively. Each of these files would contain one type of extracted record, and the number of such files depend on the number of record types recognized in the input file.
