
# Recipe to deploy on the specific Toradex Verdin iMX8 MP board

def modify_image_json(input_file, output_file, files_to_add=None):
    import json
    import sys
    """
    Modify image.json to add filelist and set autoinstall to true

    Args:
        input_file: Path to input image.json
        output_file: Path to output image.json
        files_to_add: List of files to add to filelist (default: verdin-imx8mp.itb, boot.scr)
    """
    if files_to_add is None:
        files_to_add = ["verdin-imx8mp.itb", "boot.scr"]

    # Read the original JSON
    with open(input_file, 'r') as f:
        data = json.load(f)

    # Set autoinstall to true
    data['autoinstall'] = True

    # Navigate to the partition content and add filelist
    for blockdev in data.get('blockdevs', []):
        if blockdev.get('name') == 'emmc':
            for partition in blockdev.get('partitions', []):
                content = partition.get('content', {})
                if content.get('label') == 'otaroot':
                    # Add filelist to content
                    content['filelist'] = files_to_add
                    print(f"Added filelist: {files_to_add}")

    # Write the modified JSON
    with open(output_file, 'w') as f:
        json.dump(data, f, indent=4)

    print(f"Modified JSON written to: {output_file}")

def parse_overlays_file(overlays_file):
    import re
    """Parse overlays.txt and extract overlay file names"""
    with open(overlays_file, 'r') as f:
        content = f.read().strip()

    # Extract the overlay names from fdt_overlays=overlay1.dtbo overlay2.dtbo
    match = re.search(r'fdt_overlays\s*=\s*(.+)', content)
    if not match:
        print("Error: No fdt_overlays line found in overlays.txt")
        return []

    overlays = match.group(1).strip().split()
    return overlays


def generate_overlay_entries(overlays, overlays_path, base_load_addr=0x52200000):
    """Generate ITS entries for each overlay"""
    entries = []

    for idx, overlay in enumerate(overlays):
        overlay_name = overlay.replace('.dtbo', '').replace('_overlay', '')
        entry = f'''
        fdt-{idx} {{
            description = "{overlay_name} overlay";
            data = /incbin/("{overlays_path}/{overlay}");
            type = "flat_dt";
            arch = "arm64";
            compression = "none";
            load = <{hex(base_load_addr)}>;
        }};'''
        entries.append(entry)

    return '\n'.join(entries)


def generate_fdt_list(overlays):
    """Generate the FDT list for configuration section"""
    fdt_refs = ['fdt_linux'] + [f'fdt-{idx}' for idx in range(len(overlays))]
    return ', '.join(f'"{ref}"' for ref in fdt_refs)


def process_its_template(template_file, output_file, overlays_file, overlays_path):
    import sys
    import re
    from pathlib import Path

    # Parse overlays
    overlays = parse_overlays_file(overlays_file)

    if not overlays:
        print("Warning: No overlays found, generating ITS without overlays")
    else:
        print(f"Found {len(overlays)} overlays: {', '.join(overlays)}")

    # Read template
    with open(template_file, 'r') as f:
        template_content = f.read()

    # Generate overlay entries
    overlay_entries = generate_overlay_entries(overlays, overlays_path)

    # Replace placeholder with overlay entries
    modified_content = re.sub(
        r'/\* FDT_OVERLAYS_PLACEHOLDER_START \*/.*?/\* FDT_OVERLAYS_PLACEHOLDER_END \*/',
        f'/* FDT_OVERLAYS_PLACEHOLDER_START */{overlay_entries}\n\t\t/* FDT_OVERLAYS_PLACEHOLDER_END */',
        template_content,
        flags=re.DOTALL
    )

    # Update FDT configuration
    if overlays:
        fdt_list = generate_fdt_list(overlays)
        # Replace the simple fdt line with the expanded list
        modified_content = re.sub(
            r'(\s+)fdt = "fdt_linux";',
            f'\\1fdt = {fdt_list};',
            modified_content
        )

    # Write output
    with open(output_file, 'w') as f:
        f.write(modified_content)

    print(f"Generated ITS file: {output_file}")
    print(f"Overlays included: {len(overlays)}")


def do_itb_verdin_imx8mp(d):
    """Generate ITB with overlays"""
    import subprocess
    import os

    IB_PLATFORM = d.getVar("IB_PLATFORM")
    IB_ITB_PATH = d.getVar("IB_ITB_PATH")
    IB_TARGET_ITS = d.getVar("IB_TARGET_ITS") or f"{IB_PLATFORM}_torizon"
    IB_TORIZON_DIR_PATH = d.getVar("IB_TORIZON_DIR_PATH")

    # Get the base directory for deploy images
    DEPLOY_DIR_IMAGE = f"{IB_TORIZON_DIR_PATH}/build/deploy/images/{IB_PLATFORM}"

    # Paths for overlays
    overlays_path = f"{DEPLOY_DIR_IMAGE}/overlays"
    overlays_txt_path = f"{DEPLOY_DIR_IMAGE}/overlays.txt"

    its_template = f"{IB_ITB_PATH}/{IB_TARGET_ITS}.its"
    its_output = f"{IB_ITB_PATH}/{IB_TARGET_ITS}_generated.its"
    itb_output = f"{IB_ITB_PATH}/{IB_TARGET_ITS}.itb"

    # Check if template exists
    if not os.path.isfile(its_template):
        bb.fatal(f"No corresponding ITS found for platform {IB_PLATFORM}_torizon at {its_template}")

    # Check if overlays.txt exists
    if os.path.isfile(overlays_txt_path):
        # Process the template with overlays
        process_its_template(its_template, its_output, overlays_txt_path, overlays_path)
        its_to_use = its_output
    else:
        bb.warn(f"No overlays.txt found at {overlays_txt_path}, building ITB without overlays")
        its_to_use = its_template

    # Build the ITB
    try:
        subprocess.run(
            ["mkimage", "-f", its_to_use, itb_output],
            check=True,
            capture_output=True,
            text=True
        )
    except subprocess.CalledProcessError as e:
        bb.fatal(f"Failed to create ITB: {e.stderr}")



def __do_platform_deploy(d):

    import os
    import subprocess

    IB_UBOOT_PATH = d.getVar("IB_UBOOT_PATH")
    IB_DIR = d.getVar("IB_DIR")
    IB_PLATFORM = d.getVar("IB_PLATFORM")
    IB_ITB_PATH = d.getVar("IB_ITB_PATH")
    IB_BSP_PATH = d.getVar("IB_BSP_PATH")
    IB_FILESYSTEM_PATH = d.getVar("IB_FILESYSTEM_PATH")
    IB_TORIZON_DIR_PATH = d.getVar("IB_TORIZON_DIR_PATH")
    DEPLOY_DIR_IMAGE = f"{IB_TORIZON_DIR_PATH}/build/deploy/images/{IB_PLATFORM}"

    torizon_rootfs_ar_path = __torizon_rootfs_archive_path(d)
    tezi_img_name = f"torizon-docker-{IB_PLATFORM}-Tezi"
    tezi_ar_name = f"torizon-docker-{IB_PLATFORM}-Tezi.tar"
    tezi_ar_path = f"{DEPLOY_DIR_IMAGE}/{tezi_ar_name}"
    itb_path = f"{IB_ITB_PATH}/{IB_PLATFORM}.itb"
    boot_edgem_scr_txt_path = f"{IB_BSP_PATH}/{IB_PLATFORM}/boot.edgem.scr.txt"
    bootcmd_txt_path = f"{IB_BSP_PATH}/{IB_PLATFORM}/bootcmd.txt"
    deploy_path = f"{IB_FILESYSTEM_PATH}/work/usb"

    #if not os.path.exists(deploy_path):
    #    subprocess.run(["mkdir", "-p", deploy_path])

    cmd = (f"tar -tf {torizon_rootfs_ar_path} | grep ^\./ostree/boot.1.1 | "
           f"grep -o /[a-f0-9]*/$ | sed 's/\///g\'")

    res = subprocess.run(cmd, shell=True, capture_output=True, text=True)
    deployment_id = res.stdout.strip()

    if not os.path.isfile(itb_path):
        bb.fatal(itb_path + " is missing ...")

    # Read our custom bootcmd line to be appended to the initial env later
    with open(bootcmd_txt_path) as bootcmd_txt_file:
        bootcmd_txt_content = bootcmd_txt_file.read()
        bootcmd_txt_file.close()

    # Copy the TEZI ready image tar to the deploy folder
    tezi_ar_deployed_path = f"{deploy_path}/{tezi_ar_name}"
    tezi_deployed_path = f"{deploy_path}/{tezi_img_name}"
    subprocess.run(["cp", tezi_ar_path, deploy_path])

    # Extract the tar into a known folder
    if not os.path.exists(tezi_deployed_path):
        subprocess.run(["mkdir", "-p", tezi_deployed_path])
    subprocess.run(["tar", "-p", "-xf", tezi_ar_deployed_path, "-C", tezi_deployed_path, "--strip-components=1"])

    # Copy the custom ITB
    subprocess.run(["cp", itb_path, tezi_deployed_path])

    # Copy u-boot's flash.bin to the deploy path as imx-boot
    flash_bin_path = f"{IB_UBOOT_PATH}/flash.bin"
    imx_boot_path = f"{tezi_deployed_path}/imx-boot"
    subprocess.run(["cp", flash_bin_path, imx_boot_path])

    # Directly convert the text based boot.scr into the deploy partition
    subprocess.run(["mkimage", "-A", "arm64", "-T", "script", "-C", "none", "-d", boot_edgem_scr_txt_path, f"{tezi_deployed_path}/boot.edgem.scr"])

    # Add our custom bootcmd at the end of the initial env to be imported by TEZI
    try:
        with open(f"{tezi_deployed_path}/u-boot-initial-env-sd", "a+") as dest_init_env_file:
            dest_init_env_file.write(bootcmd_txt_content)
    except Exception as e:
        __do_fs_umount(d)
        bb.fatal(f"An error has occurred: {e}")

    # Modify the image.json to enable autoinstall and add our custom files
    image_json_path = f"{tezi_deployed_path}/image.json"
    itb_file = f"{IB_PLATFORM}.itb"
    files_to_add = [itb_file, "boot.edgem.scr"]
    modify_image_json(image_json_path, image_json_path, files_to_add)

    # Finally, remove the base archive
    subprocess.run(["rm", tezi_ar_deployed_path])


python do_itb() {
    do_itb_verdin_imx8mp(d)
}