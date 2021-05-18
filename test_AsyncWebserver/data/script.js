function onButton(loc)
{
    fetch(loc+"/gpio12/on");
}

function offButton(loc)
{
    fetch(loc+"/gpio12/off");
}

