This here is a deadlock detection project, where we are foccused on building it for FreeRTOS. 
The idea is to run a random forest regressor periodically to get a score on whether a deadlock may occur.
If the prediction is higher than the threshold value.
The inference of the regressor acts as a filter for running the wfg cycle detection, this therefore helps reduce the number of wfg checks, and is helpful for larger systems.
for prediction>threshold : 1.A waitfor graph search is performed to confirm whether any cylces are present.
                       2.if cycle is confirmed a recovery task is performed to reset the entire system and force it out of deadlock 
                       by providing random delays befor mutexs are assigned.

Used emlearn to export the rf model as a c header file.

the rf model metrics :
MAE = 0.05677776278391716
R² = 0.9210394474652107

feature importance :
             Feature  Importance
3      graph_density    0.477938
2         edge_count    0.473899
1      blocked_ratio    0.022546
0      blocked_tasks    0.017201
4  mutex_utilization    0.008416

We got a score of .6965 right before the recovery was performed.
Recovery was succesfull as no further deadlocks where detected.