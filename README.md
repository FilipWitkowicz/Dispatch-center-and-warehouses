This repository contains a "Dispatcher and Warehouse project" written in C (inter-process communication in Linux system).
The project was carried out as part of the System and Concurrent Programming course.

In essence, the project involves communication between a dispatch center and warehouses. Each warehouse has 3 couriers.
Couriers receive orders from the dispatch center and then deliver raw materials from the warehouse to it.
When warehouses are empty or couriers do not receive orders for a specified period of time, the couriers terminate themselves.
When all couriers of a warehouse are no longer alive, the warehouse shuts down. When all warehouses shut down, the dispatch center also shuts down.
