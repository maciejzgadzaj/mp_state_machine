<?php

/**
 * Implements hook_views_default_views_alter().
 */
function mp_state_machine_views_default_views_alter(&$views) {
  // All columns for all line item State Machine states to the line item table
  // displayed on the admin order view page.
  if (isset($views['commerce_line_item_table'])) {
    $entity_type_state_types = mp_state_machine_entity_type_state_types();
    $handler = &$views['commerce_line_item_table']->display['default']->handler;
    $new_fields = array();

    foreach ($handler->display->display_options['fields'] as $id => $field) {
      // Add state columns before the Unit Price column.
      if ($id == 'commerce_unit_price') {
        if (!empty($entity_type_state_types['commerce_line_item'])) {
          foreach ($entity_type_state_types['commerce_line_item'] as $state_type_name => $state_type_info) {
            $new_fields[$state_type_name] = array(
              'id' => $state_type_name,
              'table' => 'views_entity_commerce_line_item',
              'field' => $state_type_name,
              'link_to_entity' => 0,
              'format_name' => 1,
            );
          }
        }
      }
      $new_fields[$id] = $field;
    }
    $handler->display->display_options['fields'] = $new_fields;
  }
}
