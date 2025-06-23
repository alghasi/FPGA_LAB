# Clone and Recreation
This project was built with vivado 2018.2, so make sure you are using this exact version.  
PL projects often come with some custom IPs, these IPs can be HDL or HLS, sth like this: 
```
ip_repo
    ├───HDL
    │   ├───HDL_IP_1
    │   └───HDL_IP_2
    └───HLS
        ├───HLS_IP_1
        └───HLS_IP_2
```

All of these IPs have their own git and are added to the project as git submodules, so to clone the project properly run: 

```  git clone --recurse-submodules <repo_url> ```

After cloning and before running the project_name.tcl to recreate the whole vivado project, firstly recreate the HLS IPs projects. 

## Recreating the HLS project
Step 1: Open Vivado HLS Command Prompt. 

Step 2: Change the directory to ip_repo\HLS folder, e.q.

``` cd c:\...\project_name\ip_repo\HLS ``` 


Step 3: source the script.tcl: 

``` vivado_hls -f HLS_IP_1\solution1\script.tcl ``` 


Step 4: Open Vivado HLS and open your recreated project. 

Step 5: Run C Synthesis and Export RTL. 

## Recreating the PL Project
Make sure all the dependencies including HLS and HDL repos are correctly placed under the right directory, then in vivado command prompt or TCL Consol of the GUI run: 

``` source c:\...\project_name\project_name.tcl ```

Wait untill recreation is completed. 

## Recreating the SDK project
Step 1: Launch Xilinx SDK not from Vivao project but instead individually. 

Step 2: Set the Workspace to ``` project_name\sdk ```

Step 3: Import the BSP, Application, and hw_platform projects into the workspace.

Step 4: Regenerate the BSP to resolve errors caused by missing .o and .a files — these are ignored in Git because they're auto-generated.

Step 5: If applicable, replace modified BSP source files (found in the custom_bsp_sources folder) with the originals in the BSP project. Then, Build All.

Step 6: Run:

``` git add --renormalize . ```

This ensures Git respects the .gitattributes file and normalizes line endings.


Refer to this [repo](https://github.com/iamhosseinali/vivado-git) and look for the right branch based on your vivado version to use vivado and git together like the project above.

