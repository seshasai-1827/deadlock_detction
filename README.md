This here is a deadlock detection project, where we are foccused on building it for FreeRTOS. 
The idea is to run a random forest regressor, in an event driven fashion(when a mutes is closed)to predict whether a deadlock may occur.
This is done to prevent constant overhead from continuously running eigther inference or WFG checking
if the prediction is higher than the threshold value.
for prediction>threshold : 1.A waitfor graph search is performed to confirm whether any cylces are present.
                       2.if cycle is confirmed a recovery task is performed to reset the entire system and force it out of deadlock 
                       by providing random delays befor mutexs are assigned.

We will run this on an STM32f4 board and confirm our results.

Used emlearn to export the rf model as a c header file.
