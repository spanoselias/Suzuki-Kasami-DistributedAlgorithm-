#!/bin/sh 
tmux new-session -d 'dir'
tmux split-window -h
tmux split-window -v 'ipython'
tmux split-window -v 'ipython'
 
tmux -2 attach-session -d 
