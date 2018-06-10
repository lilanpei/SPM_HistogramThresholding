#!/bin/bash
for i in {1..6..2}
do
 ./thread 6 $i 0.7
done
