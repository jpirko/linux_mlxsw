#!/usr/bin/python3

import csv

def csv_reader(file_obj):
    """
    Read a csv file
    """
    reader = csv.reader(file_obj)
    for row in reader:
        print(" ".join(row))


def csv_dict_reader(file_obj):
    """
    Read a CSV file using csv.DictReader
    """
    res = 0
    reader = csv.DictReader(file_obj, delimiter=',')
    for line in reader:
        print("\t{\n" +
              "\t\t.addr\t= " + line["addr"] + ",\n" +
              "\t\t.size\t= " + line["size"] + ",\n" +
              "\t},\n")
        #print(line["addr"])
        #res += int(line["size"])
        #print(res)


if __name__ == "__main__":
    with open("Spectrum3.csv") as f_obj:
        csv_dict_reader(f_obj)
        #csv_reader(f_obj)
