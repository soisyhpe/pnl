#! /bin/bash

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[0;33m'

BOLD_GREEN='\033[1;32m'
BOLD_RED='\033[1;31m'
BOLD_YELLOW='\033[1;33m'

BG_GREEN='\033[42m'
BG_RED='\033[41m'
BG_YELLOW='\033[43m'

RESET='\033[0m'

log_message() {
  # echo -e "$(date +%Y-%m-%dT%H:%M:%S) - $1${RESET}" #>> "$logfile"
  echo -e "$1${RESET}"
}

info() {
  log_message "[INFO] ${RESET}$1"
}

debug() {
  log_message "${BOLD_YELLOW}[DEBUG] ${YELLOW}$1"  
}

success() {
  log_message "${BOLD_GREEN}[SUCCESS] ${GREEN}$1"
}

error() {
  log_message "${BOLD_RED}[ERROR] ${RED}$1"
  log_message "${BOLD_RED}[ERROR] ${RED}See log file for more details."
}

# Ask for VM name
vm_name=$1
if [ -z $vm_name ]; then
  vm_name=$(read -p "VM name? " vm_name; echo $vm_name)
fi

ouichefs_bin="mkfs.ouichefs"
image_name="disk.img"
image_size="50"

# VM share directory
vm_dir="/root/pnl/vms/$vm_name"
vm_share_dir="$vm_dir/share"

# Repository directories
cd ..
root_path=$(pwd)
scripts_path="$root_path/scripts"
benchmark_path="$root_path/benchmark"
mkfs_path="$root_path/mkfs"
log_file="$root_path/script.log"

debug "vm_name = $vm_name"
debug "ouichefs_bin = $ouichefs_bin"
debug "image_name = $image_name"
debug "image_size = $image_size"
debug "vm_share_dir = $vm_share_dir"
debug "root_path = $root_path"
debug "scripts_path = $scripts_path"
debug "benchmark_path = $benchmark_path"
debug "mkfs_path = $mkfs_path"

# Module compilation
cd $root_path
info "Jumped into $root_path"

info "Started compiling module"
make debug VM=$vm_name >> "$log_file" 2>&1 # fd 2 i.e. stderr > & fd 1 i.e. stdout
if [[ $? -eq 0 ]]; then
  success "Finished compiling module successfully"
else
  error "Failed to compile module" >&2 # fd 0 i.e. stdin > & fd 2 i.e. stderr
  exit 0
fi

info "Started copying ouichefs.ko to share folder"
cp ouichefs.ko $vm_share_dir
if [[ $? -eq 0 ]]; then
  success "Finished copying ouichefs.ko to share folder"
else
  error "Failed to copy ouichefs.ko to share folder" >&2
  exit 0
fi

# Image and $ouichefs_bin
cd $mkfs_path
info "Jumped into $mkfs_path"

info "Started compiling $ouichefs_bin"
make >> "$log_file" 2>&1
if [[ $? -eq 0 ]]; then
  success "Finished compiling $ouichefs_bin"
else
  error "Failed to compile $ouichefs_bin" >&2
  exit 0
fi

info "Started applying u+x permission to executable $ouichefs_bin"
chmod u+x $ouichefs_bin
if [[ $? -eq 0 ]]; then
  success "Finished applying u+x permission to executable $ouichefs_bin"
else
  error "Failed to apply u+x permission to executable $ouichefs_bin" >&2
  exit 0
fi

info "Started building $image_name"
make img BIN=$ouichefs_bin IMG=$image_name IMGSIZE=$image_size >> "$log_file" 2>&1
if [[ $? -eq 0 ]]; then
  success "Finished building $image_name"
else
  error "Failed to build $image_name" >&2
  exit 0
fi

info "Started copying $image_name to share folder"
cp $image_name $vm_share_dir
if [[ $? -eq 0 ]]; then
  success "Finished copying $image_name to share folder"
else
  error "Failed to copy $image_name to share folder" >&2
  exit 0
fi

# Benchmark compilation
cd $benchmark_path
info "Jumped into $benchmark_path"

# info "Started compiling benchmark"
# make >> "$log_file" 2>&1
# if [[ $? -eq 0 ]]; then
#   success "Finished compiling benchmark"
# else
#   error "Failed to compile benchmark" >&2
#   exit 0
# fi


info "Started copying benchmark to share folder"
cp benchmark.c $vm_share_dir
if [[ $? -eq 0 ]]; then
  success "Finished copying benchmark to share folder"
else
  error "Failed to copy benchmark to share folder" >&2
  exit 0
fi

info "Started copying benchmark's Makefile to share folder"
cp Makefile $vm_share_dir
if [[ $? -eq 0 ]]; then
  success "Finished copying benchmark's Makefile to share folder"
else
  error "Failed to copy benchmark's Makefile to share folder" >&2
  exit 0
fi

# info "Started applying u+x permission to benchmark"
# chmod u+x $vm_share_dir/benchmark
# if [[ $? -eq 0 ]]; then
#   success "Applied u+x permission to benchmark"
# else
#   error "Failed to apply u+x permission to benchmark" >&2
#   exit 0
# fi

info "Started copying mini_script.sh to share folder"
cp $scripts_path/mini_script.sh $vm_share_dir
if [[ $? -eq 0 ]]; then
  success "Finished copying mini_script.sh to share folder"
else
  error "Failed to copy mini_script to share folder" >&2
  exit 0
fi

info "Started applying u+x permission to mini_script.sh"
chmod u+x $vm_share_dir/mini_script.sh
if [[ $? -eq 0 ]]; then
  success "Applied u+x permission to mini_script.sh"
else
  error "Failed to apply u+x permission to mini_script.sh" >&2
  exit 0
fi

info "Started copying fileinfo_user.c to share folder"
cp $root_path/fileinfo_user.c $vm_share_dir
if [[ $? -eq 0 ]]; then
  success "Finished copying fileinfo_user.c to share folder"
else
  error "Failed to copy fileinfo_user.c to share folder" >&2
  exit 0
fi

info "Started copying defrag_user.c to share folder"
cp $root_path/defrag_user.c $vm_share_dir
if [[ $? -eq 0 ]]; then
  success "Finished copying defrag_user.c to share folder"
else
  error "Failed to copy defrag_user.c to share folder" >&2
  exit 0
fi

# info "Next steps:"
# info "\t- project-run\t\tTo start VM"
# info "\t- ./mini-script.sh\tTo insert module and mount dedicated directory"
