import getopt
import os
import sys
import json

from script.config import Config
from script.extractor.csv import CSVExtractor
from script.extractor.timeData import TimeDataExtractor
from script.extractor.graphData import GraphDataExtractor
from script.graph.timeGraph import TimeGraph
from script.graph.topologyGraph import TopologyGraph


from time import localtime, strftime

# Main function
def main(argv=None):

    draw_flag = False
    hyper_option = True
    draw_option = 2

    # get arguments
    try:
        opts, args = getopt.getopt(sys.argv[1:], 'ht:g:', ['help', 'test=', 'graph='])
    except getopt.GetoptError as err:
        print (err)
        sys.exit(1)

    for o, a in opts:
        if o in ('-h', '--help'):
            usage()
            sys.exit()
        elif o in ('-t', '--test'):
            test_directory = a
        elif o in ('-g', '--graph'):
            draw_flag = True
            draw_option = int(a)
        else:
            assert False, 'unhandled option'

    if test_directory == None:
        usage()
        sys.exit()

    if 'off' in test_directory:
        hyper_option = False

    date = strftime("%m%d_%H%M", localtime())

    CSVExtractor(test_directory, date, hyper_option)
    graph_data_result, graph_file_name = GraphDataExtractor(test_directory, date, hyper_option)
    #time_data_result, time_file_name = TimeDataExtractor(test_directory, date, hyper_option)
    
    TopologyGraph(graph_data_result, graph_file_name, hyper_option)
    #TimeGraph(time_data_result, time_file_name, hyper_option)
    
if __name__ == "__main__":
    sys.exit(main())