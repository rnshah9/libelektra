package org.libelektra.exception;

import org.libelektra.Key;

public class InterfaceException extends LogicalException
{
	private static final long serialVersionUID = 1L;
	private static final String ERROR_NUMBER = "C01320";

	public InterfaceException (Key k)
	{
		super (k);
	}

	public static String errorNumber ()
	{
		return ERROR_NUMBER;
	}
}
