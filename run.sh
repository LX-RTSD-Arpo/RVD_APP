#!/bin/bash

ip_address=$(ip addr show eth1 | grep 'inet ' | awk '{print $2}' | cut -d/ -f1)
echo "Eth1 IP Address: $ip_address"

#read -p "Enter Radar IP Address: " arg1
config_file="/root/RVD/config.txt"
program_name="/root/RVD/sources/rvd-v1.0.0b1"
command="$program_name $config_file $ip_address"
max_attempts=99
attempt=1

run_rvd() {
	current_dir=$(pwd)
	echo "Current Directory: $current_dir"

	source_files="$current_dir/sources"
	echo "Sourcing files from: $source_files"

	echo "Starting $program_name..."
	$command
	return $?
}

if pgrep -f "$program_name" > /dev/null; then
	echo "Program is running"
else
	echo "Program is not running"

	while [ $attempt -le $max_attempts ]; do
		echo "Attempt $attempt"

		run_rvd
		exit_status=$?

		if [ $exit_status -eq -1 ]; then
			echo "$program_name returned -1, Restarting"
		else
			echo "$program_name exit status: $exit_status"
		fi

		((attempt++))
	done

	if [ $attempt -gt $max_attempts ]; then
		echo "Restart Failed"
	else
		echo "Restart Successfully"
	fi
fi
