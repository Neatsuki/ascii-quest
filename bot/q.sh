#!/bin/bash
# q.sh key [key...] : send keys to the "quest" tmux pane, then print the screen.
# Key names: w a s d Enter Space F5 Escape Tab Up Down Left Right etc.
for k in "$@"; do
  tmux send-keys -t quest "$k"
  sleep 0.35
done
sleep 0.7
tmux capture-pane -p -t quest
