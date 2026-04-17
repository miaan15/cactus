package ecs

import "cactus:strat"

EntityID :: distinct i32
SignatureID :: distinct i32

EntityStatus :: struct {
	signature: SignatureID,
	row:       int,
}

World :: struct {}
