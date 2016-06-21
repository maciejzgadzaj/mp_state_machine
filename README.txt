
DESCRIPTION
------------

Module extends the functionality provided by State Machine module
(https://www.drupal.org/project/state_machine), in a similar way
as the "state_flow" does it (submodule of the "state_machine" module).

The MP State Machine provides the following features:

  * provides an option to have unlimited number of state types
    (for example, in case of orders: checkout state, payment state,
    fulfillment state etc),
  * provides an option to assign a state type to any existing entity type,
  * comes pre-packaged with "checkout state" and "payment state" types
    defined on commerce_order and commerce_line_item entity types
    (where "checkout_state" replicates Drupal Commerce order statuses),
  * provides hooks to allow altering pre-defined state types, states
    and events, and defining new ones,
  * for state types common to orders and line items, when an order state
    is updated, its line item counterpart is updated at the same time
    (providing that it has the same value as the order state),
  * alters admin order view page to display all available states
    for order and its line items,
  * provides "After state transition occurs" Rules event
    for each relevant entity type,
  * provides basic Views integration.

Please note that this module by no means is supposed to be a generic
contrib module (therefore it will not be published on drupal.org),
although it should be generic enough to be used on other projects.


PRE-DEFINED STATE TYPES, STATES AND EVENTS
------------------------------------------

  * State type: checkout_state
    Available states:
    - all statuses returned by commerce_order_statuses()

  * State type: payment_state
    Available states:
    - unpaid
    - initialized
    - completed
    - validated
    - refused

  * Each state also has its own "to_<state_name>" event defined.


USAGE
-----

  $order = commerce_order_load(<order_id>);
  $state_machine = mp_state_machine_load('commerce_order', $order, MP_STATE_MACHINE_CHECKOUT_STATE);
  $state_machine->fire_event('to_processed');
  var_dump($state_machine->get_current_state());

  $line_item = commerce_line_item_load(<line_item_id>);
  $state_machine = mp_state_machine_load('commerce_line_item', $line_item, MP_STATE_MACHINE_PAYMENT_STATE);
  $state_machine->fire_event('to_completed', 'Manually updated line item payment state to completed.');
  var_dump($state_machine->get_current_state());


MORE INFORMATION
----------------

  * Commerce 2.x Stories: Workflows
    https://drupalcommerce.org/blog/43169/commerce-2x-stories-workflows

  * Drupal Commerce GitBook: State Machine
    http://docs.drupalcommerce.org/v2/dependencies/state-machine.html

  * Extending Your Workflow with State Machine
    https://www.phase2technology.com/blog/extending-your-workflow-with-state-machine/

  * Better Workflows with State Machine 2.x
    https://www.phase2technology.com/blog/better-workflows-with-state-machine-2-x/

  * Using State Machine with State Flow to build powerful workflows
    https://www.phase2technology.com/blog/using-state-machine-with-state-flow-to-build-powerful-workflows/
