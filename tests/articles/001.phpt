--TEST--
Fluent interface fun with aop
--FILE--
<?php

interface Fluent {}

class WillBeFluent implements Fluent
{
    private $stuff;

    public function setStuff($value)
    {
        $this->stuff = $value;
    }

    public function getStuff()
    {
        return $this->stuff;
    }
}

class BeFluent implements Fluent
{
    private $stuff;

    public function setStuff($value)
    {
        $this->stuff = $value;
    }

    public function getStuff()
    {
        return $this->stuff;
    }
}

class NotFluent
{
    private $stuff;

    public function setStuff($value)
    {
        $this->stuff = $value;
    }

    public function getStuff()
    {
        return $this->stuff;
    }
}

aop_add_after_returning("Fluent->set*()", function (AopJoinPoint $jp) {
    if ($jp->getReturnedValue() === null) {
        // echo "I'm updating {$jp->getMethodName()} in {$jp->getClassName()}, now returning this";
        $jp->setReturnedValue($jp->getObject());
    }
});

$beFluent = new BeFluent();
$result = $beFluent->setStuff('stuff')->setStuff('stuff2');
echo $result === $beFluent ? "ok" : "ko";

$notFluent = new NotFluent();
$result = $notFluent->setStuff('stuff');

echo $result === $notFluent ? "ko" : "ok";
?>
--EXPECT--
okok
