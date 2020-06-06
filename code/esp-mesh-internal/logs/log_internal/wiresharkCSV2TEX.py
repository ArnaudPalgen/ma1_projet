import csv
import sys

in_file = sys.argv[1]
out_file = sys.argv[2]
#"No.","Time","Source","Destination","Protocol","Length","Info"
#\ADDDEV{ESP1}{4}
#\ADDDEV{ESP2}{4}
#\ADDDEV{ROUTER}{4}
#\ADDDEV{BROADCAST}{4}
#\MESSAGE{1}{4}{ARP}{[BROADCAST] ARP for 192.168.4.1}{}{0}
mapping = {}
#esp1
mapping["Espressi_0d:7e:1d"]="1"
mapping["Espressi_0d:7e:1c"]="1"
mapping["Espressi_0d:7e:1c (3c:71:bf:0d:7e:1c) (RA)"] = "1"

#esp2
mapping["Espressi_0d:83:08"]="2"
mapping["Espressi_0d:83:09"]="2"
mapping["Espressi_0d:83:08 (3c:71:bf:0d:83:08) (RA)"]="2"
mapping["Espressi_0d:83:09 (3c:71:bf:0d:83:09) (RA)"]="2"

#router
mapping["SitecomE_0b:d4:53"]="3"
mapping["SitecomE_0b:d4:53 (64:d1:a3:0b:d4:53) (RA)"]="3"

#broadcast or no source
mapping["Broadcast"]="4"
mapping[""] = "4"

result = open(out_file, "w")
result.write("\ADDDEV{ESP1}{4}\n\ADDDEV{ESP2}{4}\n\ADDDEV{ROUTER}{4}\n\ADDDEV{BROADCAST}{4}\n")
with open(in_file, newline='\n') as csvfile:
    reader = csv.DictReader(csvfile)
    for row in reader:
        result.write("\MESSAGE{"+mapping[row["Source"]]+"}{"+mapping[row["Destination"]]+"}{"+row["Protocol"]+"}{"+row["Info"]+"}{}{0}\n")
result.close()
