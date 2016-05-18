@page global_architecture System Architecture

Global Software Architecture
============================

Several images are available:
- the bootloader in charge of boot and reset management. For more information see @ref bootloader_doc.
- the main application that runs @ref zephyr_os and @ref system_architecture "Curie&trade; BSP Software Stack.

All the images are stored in on-die non-volatile memory. For more information on image location see @ref flash_partitioning "Non Volatile Memory partitioning".

Volatile memory is also share between the images and the cores. For more information @ref ram_partitioning "Volatile Memory partitioning".

These images can be updated through different mechanisms:
<table>
<tr><th><b>Image</b>     <th><b>UART</b> <th><b>JTAG</b> <th><b>USB</b> <th><b>OTA (*)</b>
<tr><th>Bootloader FSBL  <td> No         <td> Yes        <td> Yes       <td> No
<tr><th>Bootloader SSBL  <td> Yes        <td> Yes        <td> No        <td> No
<tr><th>Main application <td> No         <td> No         <td> Yes       <td> Yes
</table>

(*) OTA update is an optional feature.


