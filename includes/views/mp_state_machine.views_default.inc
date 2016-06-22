<?php

/**
 * Implements hook_views_default_views_alter().
 */
function mp_state_machine_views_default_views_alter(&$views) {
  $entity_type_state_types = mp_state_machine_entity_type_state_types();

  // Add State Machine states columns to admin 'Line items' view
  // displayed on the admin order view page.
  if (isset($views['commerce_line_item_table'])) {
    $handler = &$views['commerce_line_item_table']->display['default']->handler;
    $new_fields = array();

    foreach ($handler->display->display_options['fields'] as $id => $field) {
      // Add state columns before the 'Unit Price' column.
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

  // Add State Machine state columns to admin 'Orders' view.
  if (isset($views['commerce_orders'])) {
    $handler = &$views['commerce_orders']->display['default']->handler;
    $new_fields = array();

    foreach ($handler->display->display_options['fields'] as $id => $field) {
      // Add state columns before the 'Operations' column.
      if ($id == 'operations') {
        if (!empty($entity_type_state_types['commerce_order'])) {
          foreach ($entity_type_state_types['commerce_order'] as $state_type_name => $state_type_info) {
            $new_fields[$state_type_name] = array(
              'id' => $state_type_name,
              'table' => 'views_entity_commerce_order',
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

  // Add State Machine state columns to admin 'Order revisions' view.
  if (isset($views['commerce_order_revisions'])) {
    $handler = &$views['commerce_order_revisions']->display['default']->handler;
    $new_fields = array();

    foreach ($handler->display->display_options['fields'] as $id => $field) {
      // Add state columns before the 'Log message' column.
      if ($id == 'log') {
        if (!empty($entity_type_state_types['commerce_order'])) {
          foreach ($entity_type_state_types['commerce_order'] as $state_type_name => $state_type_info) {
            $new_fields[$state_type_name] = array(
              'id' => $state_type_name,
              'table' => 'commerce_order_revision',
              'field' => $state_type_name,
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
