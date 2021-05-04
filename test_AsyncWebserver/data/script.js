function onButton(loc)
{
    fetch(loc+"/gpio15/on");
}

function offButton(loc)
{
    fetch(loc+"/gpio15/off");
}