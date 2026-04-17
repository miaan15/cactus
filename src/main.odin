package main

import "ecs"
import "strat"

main :: proc() {
	sm := strat.slotmap_new(int)
	strat.slotmap_append(&sm, 2)
}
