var Module = {
    canvas: document.createElement('canvas')
};
Module.canvas.id = 'canvas';
// prevent right click
Module.canvas.oncontextmenu = function(e) { e.preventDefault(); };
Module.canvas.tabIndex = -1;
document.addEventListener('DOMContentLoaded', function() {
    document.body.appendChild(Module.canvas);
});
