async function setParam(root, varName, value) {
    const url = `/${encodeURIComponent(root)}/${encodeURIComponent(varName)}?val=${encodeURIComponent(value)}`;
    const r = await fetch(url);
    if (!r.ok) {
        const txt = await r.text().catch(() => "");
        throw new Error(`${r.status} ${r.statusText} ${txt}`);
    }
    return r.text();
}


function bindRange(id) {
    const el = document.getElementById(id);
    const out = document.getElementById(id + "_val");
    const update = () => (out.textContent = el.value);
    el.addEventListener("input", update);
    update();
    return el;
}

window.addEventListener("DOMContentLoaded", () => {

    // Camera control
    const exposure = bindRange("exposure");
    const gain = bindRange("gain");
    const brightness = bindRange("brightness");
    const contrast = bindRange("contrast");

    const applyBtnControl = document.getElementById("apply_control");
    const resetBtnControl = document.getElementById("reset_control");
    const statusControl = document.getElementById("status_control");

    applyBtnControl.addEventListener("click", async () => {
        statusControl.textContent = "status: applying...";
        try {
            await setParam("camera", "exposure", exposure.value);
            await setParam("camera", "gain", gain.value);
            await setParam("camera", "brightness", brightness.value);
            await setParam("camera", "contrast", contrast.value);
            statusControl.textContent = "status: OK";
        } catch (e) {
        statusControl.textContent = "status: ERROR -> " + e.message;
        }
    });
    
    resetBtnControl.addEventListener("click", () => {
        exposure.value = "0";
        gain.value = "0";
        brightness.value = "0";
        contrast.value = "0";

        // refresh labels
        document.getElementById("exposure_val").textContent = exposure.value;
        document.getElementById("gain_val").textContent = gain.value;
        document.getElementById("brightness_val").textContent = brightness.value;
        document.getElementById("contrast_val").textContent = contrast.value;
        statusControl.textContent = "status: reset values (click Apply)";
    });

    statusControl.textContent = "status: ready";

    // ROI
    const roi_x0 = bindRange("roi_x0");
    const roi_y0 = bindRange("roi_y0");
    const roi_width = bindRange("roi_width");
    const roi_height = bindRange("roi_height");

    const applyBtnROI = document.getElementById("apply_roi");
    const resetBtnROI = document.getElementById("reset_roi");
    const statusROI = document.getElementById("status_roi");

    applyBtnROI.addEventListener("click", async () => {
        statusROI.textContent = "status: applying...";
        try {
            await setParam("roi", "x0", roi_x0.value);
            await setParam("roi", "y0", roi_y0.value);
            await setParam("roi", "dx", roi_width.value);
            await setParam("roi", "dy", roi_height.value);
            statusROI.textContent = "status: OK";
        } catch (e) {
        statusROI.textContent = "status: ERROR -> " + e.message;
        }
    });
    
    resetBtnROI.addEventListener("click", () => {
        roi_x0.value = "0";
        roi_y0.value = "0";
        roi_width.value = "50";
        roi_height.value = "50";

        // refresh labels
        document.getElementById("roi_x0_val").textContent = roi_x0.value;
        document.getElementById("roi_y0_val").textContent = roi_y0.value;
        document.getElementById("roi_width_val").textContent = roi_width.value;
        document.getElementById("roi_height_val").textContent = roi_height.value;
        statusROI.textContent = "status: reset values (click Apply)";
    });

    const roiEnableCheckbox = document.getElementById("roiEnable");

    roiEnableCheckbox.addEventListener("change", () => {
        const enabled = roiEnableCheckbox.checked ? 1 : 0;

        fetch(`/roi/enable?val=${enabled}`)
            .then(response => {
            if (!response.ok) {
                console.error("Failed to set ROI enable");
            }
            })
            .catch(err => console.error(err));
    });

    statusROI.textContent = "status: ready";
});

const img = document.getElementById("jpg");
function refreshImage() {
    img.onload = () => {
        setTimeout(refreshImage, 1000);
    };
    img.src = "/preview?t=" + Date.now();
}

refreshImage();