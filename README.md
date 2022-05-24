# CS723
low_cost_frequency_relay
 
It implement three tasks: FrequencyVirtualISRTask, FrequencyUpdaterTask and loadManagerTask. 

Timestamp is the virtual global system time.

FrequencyVirtualISRTask generate the timestamp and frequency.

FrequencyUpdaterTask calculate the roc.

LoadMangerTask maintains three modes: loadManagerState, load managing state and normal.

It has a problem inside the vTimer500MSCallback function.

400-900 timestamp show the unload and reload process
1840-2000 timestamp show how to enter into the maintenance state

The input data is the same as the data in the chart.
