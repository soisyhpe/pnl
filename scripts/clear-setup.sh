if [ -z "$1" ]; then
  echo "Usage: $0 <vm-name>"
  exit -1
fi

test -d ~/pnl/vms/$1 && rm -r ~/pnl/vms/$1
