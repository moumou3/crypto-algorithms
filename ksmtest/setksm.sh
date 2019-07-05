#!/bin/zsh

systemctl stop ksmtuned.service
echo 1 | tee /sys/kernel/mm/ksm/run

