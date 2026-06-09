module;

export module burningfloor.input;

import burningfloor.common;

export namespace bf {

InputAction move_input_action;
InputAction attack_input_action;

auto handle_input_init() {
    move_input_action = InputAction::make();
    attack_input_action = InputAction::make();

    move_input_action.add_binding_method(UDLRInputBindingMethod::make_use_arrows());
    move_input_action.add_binding_method(UDLRInputBindingMethod::make_use_wasd());
    attack_input_action.add_binding_method(ButtonInputBindingMethod{Scancode::SPACE});
}

auto handle_input_reset() {
    move_input_action.reset();
    attack_input_action.reset();
}

auto handle_input_receive_inputs() {
    move_input_action.receive_inputs();
    attack_input_action.receive_inputs();
}

auto handle_input_destroy() {
    move_input_action.destroy();
    attack_input_action.destroy();
}

} // namespace bf
