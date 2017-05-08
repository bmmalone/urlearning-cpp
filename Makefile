.PHONY: all clean astar astar-debug bfbnb bfbnb-debug score score-debug bfbnb-hash bfbnb-hash-debug

all: score astar bfbnb bfbnb-hash

clean:
	bazel clean
	
score:
	bazel build urlearning/score/score
	
score-debug:
	bazel build -c dbg urlearning/score/score
	
astar:
	bazel build urlearning/astar/astar
	
astar-debug:
	bazel build -c dbg urlearning/astar/astar
	
anytime-window-astar:
	bazel build urlearning/anytime_window_astar/anytime-window-astar
	
anytime-window-astar-debug:
	bazel build -c dbg urlearning/anytime_window_astar/anytime-window-astar
	
bfbnb:
	bazel build urlearning/bfbnb/bfbnb
	
bfbnb-debug:
	bazel build -c dbg urlearning/bfbnb/bfbnb
	
bfbnb-hash:
	bazel build urlearning/bfbnb_hash/bfbnb-hash
	
bfbnb-hash-debug:
	bazel build -c dbg urlearning/bfbnb_hash/bfbnb-hash

