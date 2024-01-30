#!/usr/bin/env python3

import requests
import time
from concurrent.futures import ThreadPoolExecutor
import argparse

total_bytes = 0
start_time = time.time()

def fetch_url(url):
    response = requests.get(url)
    try:
        global total_bytes
        total_bytes += len(response.text)
        print("%.3f kbyte/s" % ((total_bytes*1.0e-3)/(time.time() - start_time)))
    except Exception as ex:
        print(ex)
        pass
    return response.status_code

parser = argparse.ArgumentParser(description="HTTP Benchmark")
parser.add_argument("url", type=str, help="URL to fetch")
parser.add_argument("--times", type=int, default=10, help="Number of times to fetch the URL")
parser.add_argument("--parallel", type=int, default=1, help="Number of parallel requests")

args = parser.parse_args()

start_time = time.time()

with ThreadPoolExecutor(max_workers=args.parallel) as executor:
    futures = [executor.submit(fetch_url, args.url) for _ in range(args.times)]
    for future in futures:
        future.result()

end_time = time.time()
print("Got %.3f ops/s" % (args.times/(end_time-start_time)))
