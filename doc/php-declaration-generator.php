<?php
/** @noinspection PhpUnused */
/** @noinspection PhpUnhandledExceptionInspection */

const T = '    ';
const N = PHP_EOL;

// Generate a PHP declaration for the given type.
$functions = ['aop_add_around', 'aop_add_before', 'aop_add_after', 'aop_add_after_returning', 'aop_add_after_throwing'];
$classes = ['AopJoinPoint'];
$constant_prefix = 'AOP';



// Beginning
$php = '<?php' . N . N;
$php .= '/**' . N . ' * Generated stub file for code completion purposes' . N . ' */';
$php .= N;



// Constant definitions
$constants = [];
foreach (get_defined_constants() as $cname => $cvalue) {
    if (str_starts_with($cname, $constant_prefix)) {
        $constants[$cname] = $cvalue;
    }
}

asort($constants);

foreach ($constants as $cname => $cvalue) {
    $php .= 'define(\'' . $cname . '\', ' . $cvalue . ');' . N;
}

$php .= N;



// Functions
foreach ($functions as $function) {
    /** @noinspection PhpUnhandledExceptionInspection */
    $refl = new ReflectionFunction($function);
    $php .= 'function ' . $refl->getName() . '(';
    foreach ($refl->getParameters() as $i => $parameter) {
        if ($i >= 1) {
            $php .= ', ';
        }
        if (($type = $parameter->getType()) === 'class') {
            $php .= $type->getName() . ' ';
        }
        $php .= '$' . $parameter->getName();
        if ($parameter->isDefaultValueAvailable()) {
            $php .= ' = ' . $parameter->getDefaultValue();
        }
    }
    $php .= ') {}' . N;
}

$php .= N;



// Classes
foreach ($classes as $class) {
    $refl = new ReflectionClass($class);

    // Class definition
    $php .= 'class ' . $refl->getName();
    if ($parent = $refl->getParentClass()) {
        $php .= ' extends ' . $parent->getName();
    }
    $php .= N . '{' . N;

    // Properties
    foreach ($refl->getProperties() as $property) {
        $php .= T . '$' . $property->getName() . ';' . N;
    }

    // Methods
    foreach ($refl->getMethods() as $method) {
        if ($method->isPublic()) {
            if ($method->getDocComment()) {
                $php .= T . $method->getDocComment() . N;
            }
            $php .= T . 'public function ';
            if ($method->returnsReference()) {
                $php .= '&';
            }
            $php .= $method->getName() . '(';
            foreach ($method->getParameters() as $i => $parameter) {
                if ($i >= 1) {
                    $php .= ', ';
                }
                if ($parameter->getType() === 'array') {
                    $php .= 'array ';
                }
                if (($type = $parameter->getType()) === 'class') {
                    $php .= $type->getName() . ' ';
                }
                $php .= '$' . $parameter->getName();
                if ($parameter->isDefaultValueAvailable()) {
                    $php .= ' = ' . $parameter->getDefaultValue();
                }
            }
            $php .= ') {}' . N;
        }
    }
    $php .= '}';
}

echo $php . N;