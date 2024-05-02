import os
import subprocess

relative_script_path = "generate_html_header.sh"
absolute_script_path = os.path.abspath(os.path.join(os.getcwd(), relative_script_path))

# Aufruf des Bash-Skripts
subprocess.call(["bash", absolute_script_path])